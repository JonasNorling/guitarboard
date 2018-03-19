/*
 * Platform specific details for running on STM32F405RG.
 */

#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencmsis/core_cm3.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include "platform.h"
#include "wm8731.h"
#include "codec.h"
#include "usb.h"

#define ADC_PINS KNOB_COUNT

// STM32F405RG pin map
static const struct {
    uint32_t port;
    uint16_t pinno;
    uint8_t adcno;
} pins[KNOB_COUNT] = {
        { GPIOA, GPIO0, 0 },
        { GPIOA, GPIO1, 1 },
        { GPIOA, GPIO2, 2 },
        { GPIOA, GPIO3, 3 },
        { GPIOC, GPIO0, 10 },
        { GPIOC, GPIO1, 11 },
};

static const KnobConfig defaultKnobConfig[KNOB_COUNT] = {
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
};

static const uint32_t ADC_DMA_STREAM = DMA_STREAM0;
static const uint32_t ADC_DMA_CHANNEL = DMA_SxCR_CHSEL_0;

static volatile uint16_t adcSamples[ADC_PINS]; // 12-bit value
static volatile uint16_t adcValues[ADC_PINS]; // 16-bit value

static volatile int frameTicksMin, frameTicksMax;

static void(*idleCallback)(void);

static void adcInit(const KnobConfig* knobConfig)
{
    // Set up analog input pins and build a channel list
    uint8_t channels[ADC_PINS];
    size_t channelCount = 0;
    for (size_t i = 0; i < KNOB_COUNT; i++) {
        if (knobConfig[i].analog) {
            gpio_mode_setup(pins[i].port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, pins[i].pinno);
            channels[channelCount++] = pins[i].adcno;
        }
    }

    if (channelCount == 0) {
        return;
    }

    adc_power_off(ADC1);
    adc_enable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);

    adc_power_on(ADC1);

    adc_set_regular_sequence(ADC1, channelCount, channels);

    // Configure the DMA engine to stream data from the ADC.
    dma_stream_reset(DMA1, ADC_DMA_STREAM);
    dma_set_peripheral_address(DMA2, ADC_DMA_STREAM, (intptr_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA2, ADC_DMA_STREAM, (intptr_t)adcSamples);
    dma_set_number_of_data(DMA2, ADC_DMA_STREAM, channelCount);
    dma_channel_select(DMA2, ADC_DMA_STREAM, ADC_DMA_CHANNEL);
    dma_set_transfer_mode(DMA2, ADC_DMA_STREAM, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_memory_size(DMA2, ADC_DMA_STREAM, DMA_SxCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA2, ADC_DMA_STREAM, DMA_SxCR_PSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA2, ADC_DMA_STREAM);
    dma_enable_circular_mode(DMA2, ADC_DMA_STREAM);
    dma_enable_stream(DMA2, ADC_DMA_STREAM);

    adc_set_dma_continue(ADC1);
    adc_set_continuous_conversion_mode(ADC1);
    adc_enable_dma(ADC1);
    adc_start_conversion_regular(ADC1);
}

static void ioInit(const KnobConfig* knobConfig)
{
    for (size_t i = 0; i < KNOB_COUNT; i++) {
        if (!knobConfig[i].analog) {
            gpio_mode_setup(pins[i].port, GPIO_MODE_INPUT,
                    knobConfig[i].pullup ? GPIO_PUPD_PULLUP : GPIO_PUPD_NONE,
                            pins[i].pinno);
        }
    }
}

void platformFrameFinishedCB(int ticks)
{
    // Lowpass filter analog inputs
    for (unsigned i = 0; i < ADC_PINS; i++) {
        adcValues[i] = adcValues[i] - (adcValues[i] >> 4) + adcSamples[i];
    }

    if (ticks < frameTicksMin) {
        frameTicksMin = ticks;
    }
    if (ticks > frameTicksMax) {
        frameTicksMax = ticks;
    }
}

void platformInit(const KnobConfig* knobConfig)
{
    if (!knobConfig) {
        knobConfig = defaultKnobConfig;
    }

    rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_DMA1);
    rcc_periph_clock_enable(RCC_DMA2);
    rcc_periph_clock_enable(RCC_ADC1);

    systick_counter_enable();
    systick_set_reload(0xffffff);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);

    // Enable LED pins and turn them on
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8 | GPIO7);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
    setLed(LED_GREEN, true);
    setLed(LED_RED, true);
    setLed(LED_BLUE, true);

    adcInit(knobConfig);
    ioInit(knobConfig);
    codecInit();
    usbInit();
}

uint16_t knob(uint8_t n)
{
    return adcValues[n];
}

bool button(uint8_t n)
{
    return gpio_get(pins[n].port, pins[n].pinno);
}

void platformRegisterIdleCallback(void(*cb)(void))
{
    idleCallback = cb;
}

void platformMainloop(void)
{
    setLed(LED_GREEN, false);
    setLed(LED_RED, false);
    setLed(LED_BLUE, false);

    unsigned lastprint = 0;

    while (true) {
        __WFI();

        if (samplecounter >= lastprint + CODEC_SAMPLERATE) {
            printf("%u samples, peak %5d %5d. ADC %x %x %x %x %x %x. Ticks %5d-%5d of 28000.\n",
                    samplecounter, peakIn, peakOut,
                    adcValues[0], adcValues[1],
                    adcValues[2], adcValues[3],
                    adcValues[4], adcValues[5],
                    frameTicksMin, frameTicksMax);

            frameTicksMin = INT32_MAX;
            frameTicksMax = INT32_MIN;
            peakIn = peakOut = INT16_MIN;
            lastprint += CODEC_SAMPLERATE;
        }

        if (idleCallback) {
            idleCallback();
        }
    }
}
