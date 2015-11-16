#pragma once

#include <stdlib.h>
#include "codec.h"

#define DELAY_LINE_LENGTH (CODEC_SAMPLERATE/25)
#define SCAN_LENGTH (DELAY_LINE_LENGTH - 2)

typedef struct {
    CodecIntSample delayline_l[DELAY_LINE_LENGTH];
    CodecIntSample delayline_r[DELAY_LINE_LENGTH];
    size_t writepos;
    float readphase[2];
} PitcherState;

typedef struct {
    float speed;// RAMP(params->pedal, 0.0f, 1.0f);
    float wet;// RAMP(params->knob1, 0.0f, 1.0f);
    float phasediff;// RAMP(params->knob3, 0.0f, 0.02f);
} PitcherParams;

/**
 * Initialize the pitch shifter effect, creating a predictable state
 *
 * @param state State structure to initialize. Should be allocated by the caller
 * and passed to subsequent calls to processPitcher().
 */
void initPitcher(PitcherState* state);

/**
 * Run the pitch shifter effect over a buffer of stereo samples.
 *
 * @param in Pointer to input samples
 * @param out Pointer to output samples
 * @param state Mutable state of the effect, such as the delay line state
 * @param param Input parameters to the effect
 */
void processPitcher(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, PitcherState* state,
        const PitcherParams* params);
