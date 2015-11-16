#pragma once

#include "codec.h"

#define VIBRATO_MAX_DEPTH 50
#define VIBRATO_LINELEN (VIBRATO_MAX_DEPTH * 2)

typedef struct {
    float delayline_l[VIBRATO_LINELEN];
    float delayline_r[VIBRATO_LINELEN];
    size_t writepos;
    float phase;
} VibratoState;

typedef struct {
    float speed;
    float depth;
    float phasediff;
} VibratoParams;

/**
 * Initialize the vibrato effect, creating a predictable state
 *
 * @param state State structure to initialize. Should be allocated by the caller
 * and passed to subsequent calls to processVibrato().
 */
void initVibrato(VibratoState* state);

/**
 * Run the vibrato effect over a buffer of stereo samples.
 *
 * @param in Pointer to input samples
 * @param out Pointer to output samples
 * @param state Mutable state of the effect, such as the delay line data
 * @param param Input parameters to the effect
 */
void processVibrato(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, VibratoState* state,
        const VibratoParams* params);
