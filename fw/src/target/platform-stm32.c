#include "platform.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

void platformInit(void)
{
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);

    // Enable LED pins and turn them on
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3 | GPIO7);
    gpio_set(GPIOC, GPIO3 | GPIO7);
}
