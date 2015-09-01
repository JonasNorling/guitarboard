#pragma once

#include "codec.h"

extern _Atomic unsigned samplecounter;
extern _Atomic CodecIntSample peakIn;
extern _Atomic CodecIntSample peakOut;

void codecInit(void);
