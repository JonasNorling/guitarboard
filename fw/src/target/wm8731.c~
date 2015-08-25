#include "codec.h"
#include "wm8731.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void codecInit(void)
{
    // I2S2 clocks and alternate function mapping
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12 | GPIO13 | GPIO14 | GPIO15);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO14 | GPIO15);
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_af(GPIOC, GPIO_AF5, GPIO6);

    // SPI1 clocks and alternate function mapping
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO4 | GPIO5 | GPIO7);
    gpio_set_af(GPIOA, GPIO_AF5, GPIO4 | GPIO5 | GPIO7);


    // Set up I2S for 48kHz 16-bit stereo.
    // I believe the input to the PLLI2S is 1MHz. Division factors are from
    // table 126 in the data sheet.

    spi_reset(SPI2);

    RCC_PLLI2SCFGR = (3 << 28) | (258 << 6);
    RCC_CR |= RCC_CR_PLLI2SON;

    const unsigned i2sdiv = 3;
    SPI_I2SPR(SPI2) = SPI_I2SPR_MCKOE | SPI_I2SPR_ODD | i2sdiv;
    SPI_I2SCFGR(SPI2) |= SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SE |
            (SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB);

    SPI_DR(SPI2) = 0xaaaa;
    SPI_DR(SPI2) = 0x5555;
}

void codecRegisterProcessFunction(CodecProcess fn)
{
    (void)fn;
}
