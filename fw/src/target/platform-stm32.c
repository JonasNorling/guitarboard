#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencmsis/core_cm3.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform.h"
#include "wm8731.h"
#include "codec.h"

static const uint32_t ADC_DMA_STREAM = DMA_STREAM0;
static const uint32_t ADC_DMA_CHANNEL = DMA_SxCR_CHSEL_0;

#define ADC_PINS 6
static volatile uint16_t adcSamples[ADC_PINS]; // 12-bit value
static volatile uint16_t adcValues[ADC_PINS]; // 16-bit value

static void adcInit(void)
{
    // Set up analog inputs
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO3);
    gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0 | GPIO1);

    adc_off(ADC1);
    adc_enable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);

    adc_power_on(ADC1);

    const uint8_t channels[ADC_PINS] = { 0, 1, 2, 3, 10, 11 };
    adc_set_regular_sequence(ADC1, ADC_PINS, (uint8_t*)channels);

    // Configure the DMA engine to stream data from the ADC.
    dma_stream_reset(DMA1, ADC_DMA_STREAM);
    dma_set_peripheral_address(DMA2, ADC_DMA_STREAM, (intptr_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA2, ADC_DMA_STREAM, (intptr_t)adcSamples);
    dma_set_number_of_data(DMA2, ADC_DMA_STREAM, ADC_PINS);
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

void platformFrameFinishedCB(void)
{
    // Lowpass filter analog inputs
    for (unsigned i = 0; i < ADC_PINS; i++) {
        adcValues[i] = adcValues[i] - (adcValues[i] >> 4) + adcSamples[i];
    }
}

void platformInit(void)
{
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_DMA1);
    rcc_periph_clock_enable(RCC_DMA2);
    rcc_periph_clock_enable(RCC_ADC1);

    // Enable LED pins and turn them on
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3 | GPIO7);
    setLed(LED_GREEN, true);
    setLed(LED_RED, true);

    adcInit();
    codecInit();
}

uint16_t knob(uint8_t n)
{
    return adcValues[n];
}

void platformMainloop(void)
{
    setLed(LED_GREEN, false);
    setLed(LED_RED, false);

    unsigned lastprint = 0;

    while (true) {
        __WFI();

        if (samplecounter >= lastprint + CODEC_SAMPLERATE) {
            printf("%u samples, peak %5u %5u. ADC %x %x %x %x %x %x\n",
                    samplecounter, peakIn, peakOut,
                    adcValues[0], adcValues[1],
                    adcValues[2], adcValues[3],
                    adcValues[4], adcValues[5]);

            peakIn = peakOut = 0;
            lastprint += CODEC_SAMPLERATE;
        }
    }
}

ssize_t _write(int fd __attribute__((unused)), const void* buf, size_t count)
{
    if (!(ITM_TER[0] & 1)) {
        // Tracing not enabled
        return count;
    }

    const char* charbuf = buf;
    for (size_t i = 0; i < count; i++) {
        while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY))
            ;
        ITM_STIM8(0) = *charbuf++;
    }
    return count;
}
