#include "platform.h"
#include "jackclient.h"

void platformInit(const KnobConfig* knobConfig)
{
    (void)knobConfig;
    jackClientInit();
}

void platformRegisterIdleCallback(void(*cb)(void))
{
    (void)cb;
}

void platformMainloop(void)
{
    jackClientRun();
}

uint16_t knob(uint8_t n)
{
    (void)n;
    return 0;
}

bool button(uint8_t n)
{
    (void)n;
    return false;
}
