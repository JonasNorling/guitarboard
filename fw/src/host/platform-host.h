#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline void setLed(enum Led led, bool state)
{
    (void)led;
    (void)state;
}

static inline uint16_t knob(uint8_t n)
{
    (void)n;
    return 0;
}
