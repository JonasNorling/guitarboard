#pragma once

#include "biquad.h"
#include "codec.h"

typedef struct {
    FloatBiquadState bqstate;
} WahwahState;

typedef struct {
    float wah; ///< peak, 0..1
    float q; ///< resonance, 0..1
} WahwahParams;

/**
 * Initialize the wah-wah effect, creating a predictable state
 *
 * @param state State structure to initialize. Should be allocated by the caller
 * and passed to subsequent calls to processWahwah().
 */
void initWahwah(WahwahState* state);

/**
 * Run the wah-wah effect over a buffer of stereo samples.
 *
 * @param in Pointer to input samples
 * @param out Pointer to output samples
 * @param state Mutable state of the effect, such as the biquad filter state
 * @param param Input parameters to the effect
 */
void processWahwah(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, WahwahState* state,
        const WahwahParams* params);
