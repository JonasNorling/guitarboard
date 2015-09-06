#pragma once
#include "codec.h"

/**
 * Biquad coefficients for
 *
 *          b0 + b1*z^-1 + b2*z^-2
 *  H(z) = ------------------------
 *          a0 + a1*z^-1 + a2*z^-2
 *
 *  Where a0 has been normalized to 1.
 */
typedef struct {
    float gain;
    float a1, a2; // poles
    float b0, b1, b2; // zeros
} FloatBiquadCoeffs;

/**
 * Stereo state for a biquad stage.
 */
typedef struct {
    float X[2][2];
    float Y[2][2];
} FloatBiquadState;

/**
 * Create a second-order resonant lowpass filter
 *
 * @param w0 Cutoff/resonance frequency in radians
 * @param q Resonance
 */
void bqMakeLowpass(FloatBiquadCoeffs* c, float w0, float q);

/**
 * Create a second-order bandpass filter
 *
 * @param w0 Centre frequency in radians
 * @param q Bandwidth/resonance figure
 */
void bqMakeBandpass(FloatBiquadCoeffs* c, float w0, float q);

/**
 * Run a biquad filter
 */
void bqProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const FloatBiquadCoeffs* c, FloatBiquadState* state);
