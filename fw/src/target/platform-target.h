#pragma once

#include <stdbool.h>
#include <libopencm3/stm32/gpio.h>

static inline void setLed(enum Led led, bool state)
{
    switch (led) {
    case LED_GREEN:
        GPIO_BSRR(GPIOC) = state ? GPIO7 : (GPIO7 << 16);
        break;
    case LED_RED:
        GPIO_BSRR(GPIOC) = state ? GPIO8 : (GPIO8 << 16);
        break;
    case LED_BLUE:
        GPIO_BSRR(GPIOB) = state ? GPIO6 : (GPIO6 << 16);
        break;
    }
}

void platformFrameFinishedCB(void);
