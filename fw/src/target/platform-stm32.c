#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencmsis/core_cm3.h>
#include <unistd.h>
#include <stdbool.h>

#include "platform.h"
#include "wm8731.h"


void platformInit(void)
{
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_DMA1);

    // Enable LED pins and turn them on
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3 | GPIO7);
    setLed(LED_GREEN, true);
    setLed(LED_RED, true);

    codecInit();
}

void platformMainloop(void)
{
    setLed(LED_GREEN, false);
    setLed(LED_RED, false);

    while (true) {
        __WFI();
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
