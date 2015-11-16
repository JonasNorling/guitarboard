#pragma once

#include "codec.h"

#define DELAY_LINELEN (CODEC_SAMPLERATE/2)

typedef struct {
    CodecIntSample delayline_l[DELAY_LINELEN];
    CodecIntSample delayline_r[DELAY_LINELEN];
    float filteredLength;
    size_t writepos;
    size_t octaverPhase;
} DelayState;

typedef struct {
    float input;
    float confusion;
    float feedback;
    float octaveMix;
    float length;
} DelayParams;

/**
 * Initialize the delay effect, creating a predictable state
 *
 * @param state State structure to initialize. Should be allocated by the caller
 * and passed to subsequent calls to processDelay().
 */
void initDelay(DelayState* state);

/**
 * Run the delay effect over a buffer of stereo samples.
 *
 * @param in Pointer to input samples
 * @param out Pointer to output samples
 * @param state Mutable state of the effect, such as the delay line data
 * @param param Input parameters to the effect
 */
void processDelay(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, DelayState* state,
        const DelayParams* params);
