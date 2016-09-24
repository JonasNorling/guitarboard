#include "codec.h"
#include "wm8731.h"
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/nvic.h>
#include "platform.h"
#include "usb_audio.h"

#include <stdint.h>
#include <string.h>

_Atomic unsigned samplecounter;
_Atomic CodecIntSample peakIn = INT16_MIN;
_Atomic CodecIntSample peakOut = INT16_MIN;

static const uint32_t DAC_DMA_STREAM = DMA_STREAM4; // SPI2_TX
static const uint32_t DAC_DMA_CHANNEL = DMA_SxCR_CHSEL_0;
static const uint32_t ADC_DMA_STREAM = DMA_STREAM3; // I2S2_EXT_RX
static const uint32_t ADC_DMA_CHANNEL = DMA_SxCR_CHSEL_3;

#define BUFFER_SAMPLES ((CODEC_SAMPLES_PER_FRAME)*2)
static int16_t dacBuffer[2][BUFFER_SAMPLES] __attribute__((aligned(4)));
static int16_t adcBuffer[2][BUFFER_SAMPLES] __attribute__((aligned(4)));

static CodecProcess appProcess;

static void codecWriteReg(unsigned reg, unsigned value)
{
    gpio_clear(GPIOA, GPIO4);
    spi_send(SPI1, (reg << 9) | value);
    while (!(SPI_SR(SPI1) & SPI_SR_TXE));
    while (SPI_SR(SPI1) & SPI_SR_BSY);

    for (int i = 0; i < 100; i++) {
        __asm__("nop");
    }

    gpio_set(GPIOA, GPIO4);

    for (int i = 0; i < 1000; i++) {
        __asm__("nop");
    }
}

static void codecConfig()
{
    codecWriteReg(0x0f, 0b000000000); // Reset!
    codedSetInVolume(0);
    codedSetOutVolume(-10);
    codecWriteReg(0x04, 0b000010010); // Analog path - select DAC, no bypass
    codecWriteReg(0x05, 0b000000001); // Digital path - disable soft mute and highpass
    codecWriteReg(0x06, 0b000000000); // Power down control - enable everything
    codecWriteReg(0x07, 0b000000010); // Interface format - 16-bit I2S
    codecWriteReg(0x09, 0b000000001); // Active control - engage!
}

void codedSetInVolume(int vol)
{
    // -23 <= vol <= 8
    const unsigned involume = 0x17 + vol;
    codecWriteReg(0x00, 0x100 | (involume & 0x1f)); // Left line in, unmute
}

void codedSetOutVolume(int voldB)
{
    // -73 <= voldB <= 6
    const unsigned volume = 121 + voldB;
    codecWriteReg(0x02, 0x100 | (volume & 0x7f)); // Left headphone
}

void codecInit(void)
{
    memset(adcBuffer, 0xaa, sizeof(adcBuffer));

    // I2S2 alternate function mapping
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO15);
    gpio_set_af(GPIOB, GPIO_AF6, GPIO14); // I2S2ext_SD (I2S receive)
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_af(GPIOC, GPIO_AF5, GPIO6); // MCLK
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO12 | GPIO13 | GPIO15);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO6);

    // Set up SPI1
    spi_reset(SPI1);

    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_256, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
            SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_16BIT, SPI_CR1_MSBFIRST);
    spi_set_nss_high(SPI1);
    spi_enable_software_slave_management(SPI1);
    spi_enable(SPI1);

    // SPI1 alternate function mapping, set CSB signal high output
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5 | GPIO7);
    gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO7);
    gpio_set(GPIOA, GPIO4);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);

    codecConfig();

    // Set up I2S for 48kHz 16-bit stereo.
    // The input to the PLLI2S is 1MHz. Division factors are from
    // table 126 in the data sheet.
    //
    // This gives us 1.536MHz SCLK = 16 bits * 2 channels * 48000 Hz
    // and 12.288 MHz MCLK = 256 * 48000 Hz.
    // With this PLL configuration the actual sampling frequency
    // is nominally 47991 Hz.

    spi_reset(SPI2);

    RCC_PLLI2SCFGR = (3 << 28) | (258 << 6);
    RCC_CR |= RCC_CR_PLLI2SON;
    while (!(RCC_CR & RCC_CR_PLLI2SRDY));

    const unsigned i2sdiv = 3;
    SPI_I2SPR(SPI2) = SPI_I2SPR_MCKOE | SPI_I2SPR_ODD | i2sdiv;
    SPI_I2SCFGR(SPI2) |= SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SE |
            (SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB);
    SPI_I2SCFGR(I2S2_EXT_BASE) = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SE |
            (SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE << SPI_I2SCFGR_I2SCFG_LSB);

    // Configure the DMA engine to stream data to the DAC.
    dma_stream_reset(DMA1, DAC_DMA_STREAM);
    dma_set_peripheral_address(DMA1, DAC_DMA_STREAM, (intptr_t)&SPI_DR(SPI2));
    dma_set_memory_address(DMA1, DAC_DMA_STREAM, (intptr_t)dacBuffer[0]);
    dma_set_memory_address_1(DMA1, DAC_DMA_STREAM, (intptr_t)dacBuffer[1]);
    dma_set_number_of_data(DMA1, DAC_DMA_STREAM, BUFFER_SAMPLES);
    dma_channel_select(DMA1, DAC_DMA_STREAM, DAC_DMA_CHANNEL);
    dma_set_transfer_mode(DMA1, DAC_DMA_STREAM, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_set_memory_size(DMA1, DAC_DMA_STREAM, DMA_SxCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA1, DAC_DMA_STREAM, DMA_SxCR_PSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA1, DAC_DMA_STREAM);
    dma_enable_double_buffer_mode(DMA1, DAC_DMA_STREAM);
    dma_enable_stream(DMA1, DAC_DMA_STREAM);

    // Configure the DMA engine to stream data from the ADC.
    dma_stream_reset(DMA1, ADC_DMA_STREAM);
    dma_set_peripheral_address(DMA1, ADC_DMA_STREAM, (intptr_t)&SPI_DR(I2S2_EXT_BASE));
    dma_set_memory_address(DMA1, ADC_DMA_STREAM, (intptr_t)adcBuffer[0]);
    dma_set_memory_address_1(DMA1, ADC_DMA_STREAM, (intptr_t)adcBuffer[1]);
    dma_set_number_of_data(DMA1, ADC_DMA_STREAM, BUFFER_SAMPLES);
    dma_channel_select(DMA1, ADC_DMA_STREAM, ADC_DMA_CHANNEL);
    dma_set_transfer_mode(DMA1, ADC_DMA_STREAM, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_memory_size(DMA1, ADC_DMA_STREAM, DMA_SxCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA1, ADC_DMA_STREAM, DMA_SxCR_PSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA1, ADC_DMA_STREAM);
    dma_enable_double_buffer_mode(DMA1, ADC_DMA_STREAM);
    dma_enable_stream(DMA1, ADC_DMA_STREAM);

    dma_enable_transfer_complete_interrupt(DMA1, ADC_DMA_STREAM);
    nvic_enable_irq(NVIC_DMA1_STREAM3_IRQ);
    nvic_set_priority(NVIC_DMA1_STREAM3_IRQ, 0x80); // 0 is most urgent

    // Start transmitting data
    spi_enable_rx_dma(I2S2_EXT_BASE);
    spi_enable_tx_dma(SPI2);
}

void dma1_stream3_isr(void)
{
    dma_clear_interrupt_flags(DMA1, ADC_DMA_STREAM, DMA_TCIF);

    AudioBuffer* outBuffer = dma_get_target(DMA1, DAC_DMA_STREAM) ?
                    (void*)dacBuffer[0] :
                    (void*)dacBuffer[1];
    AudioBuffer* inBuffer = dma_get_target(DMA1, DAC_DMA_STREAM) ?
                    (void*)adcBuffer[0] :
                    (void*)adcBuffer[1];

    // Correct DC offset
    static int32_t correction[2] = { 1230 << 16, 1230 << 16 };
    int average[2] = {};
    for (unsigned n = 0; n < CODEC_SAMPLES_PER_FRAME; n++) {
        inBuffer->s[n][0] += correction[0] >> 16;
        inBuffer->s[n][1] += correction[1] >> 16;
        average[0] += inBuffer->s[n][0];
        average[1] += inBuffer->s[n][1];
    }
    correction[0] -= average[0] << 2;
    correction[1] -= average[1] << 2;

    if (appProcess) {
        appProcess((const AudioBuffer*)inBuffer, (AudioBuffer*)outBuffer);
    }

    samplecounter += CODEC_SAMPLES_PER_FRAME;
    CodecIntSample framePeakOut = INT16_MIN;
    CodecIntSample framePeakIn = INT16_MIN;
    for (unsigned s = 0; s < BUFFER_SAMPLES; s++) {
        if (outBuffer->m[s] > framePeakOut) {
            framePeakOut = outBuffer->m[s];
        }
        if (inBuffer->m[s] > framePeakIn) {
            framePeakIn = inBuffer->m[s];
        }
    }
    if (framePeakOut > peakOut) {
        peakOut = framePeakOut;
    }
    if (framePeakIn > peakIn) {
        peakIn = framePeakIn;
    }

    platformFrameFinishedCB();
}

void codecPeek(const int16_t** buffer, unsigned* buffersamples, unsigned* writepos)
{
    *buffersamples = 2*BUFFER_SAMPLES;
    *buffer = adcBuffer[0];

    unsigned target = dma_get_target(DMA1, ADC_DMA_STREAM);
    unsigned numberOfData = DMA_SNDTR(DMA1, ADC_DMA_STREAM);

    if (target != dma_get_target(DMA1, ADC_DMA_STREAM)) {
        target = dma_get_target(DMA1, ADC_DMA_STREAM);
        numberOfData = DMA_SNDTR(DMA1, ADC_DMA_STREAM);
    }

    *writepos = (target ? BUFFER_SAMPLES : 0) + (BUFFER_SAMPLES - numberOfData);
}

void codecRegisterProcessFunction(CodecProcess fn)
{
    appProcess = fn;
}
