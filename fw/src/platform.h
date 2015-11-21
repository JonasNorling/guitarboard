#pragma once

enum Led {
    LED_RED,
    LED_GREEN
};

#define KNOB_COUNT 6

#ifdef HOST
#include "host/platform-host.h"
#else
#include "target/platform-target.h"
#endif

typedef struct {
    uint8_t analog : 1;
    uint8_t pullup : 1;
} KnobConfig;

void platformInit(const KnobConfig* knobConfig);
void platformMainloop(void);
void platformRegisterIdleCallback(void(*cb)(void));

uint16_t knob(uint8_t n);
bool button(uint8_t n);
