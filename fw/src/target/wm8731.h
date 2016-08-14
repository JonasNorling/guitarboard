#pragma once

#include "codec.h"

extern _Atomic unsigned samplecounter;
extern _Atomic CodecIntSample peakIn;
extern _Atomic CodecIntSample peakOut;

void codecInit(void);

/**
 * Get a pointer to where in the whole audio input buffer the ADC
 * DMA job is currently writing.
 */
void codecPeek(const int16_t** buffer, unsigned* buffersamples, unsigned* writepos);
