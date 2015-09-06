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
        GPIO_BSRR(GPIOC) = state ? GPIO3 : (GPIO3 << 16);
        break;
    }
}

uint16_t knob(uint8_t n);

void platformFrameFinishedCB(void);
