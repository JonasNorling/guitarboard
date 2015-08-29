#pragma once

enum Led {
    LED_RED,
    LED_GREEN
};

#ifdef HOST
#include "host/platform-host.h"
#else
#include "target/platform-target.h"
#endif

void platformInit(void);
void platformMainloop(void);
