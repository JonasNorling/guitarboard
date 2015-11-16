#include "../dsp/biquad.h"

#include <math.h>

#include "codec.h"

/*
 * Some ideas from:
 * Cookbook formulae for audio EQ biquad filter coefficients
 * by Robert Bristow-Johnson  <rbj@audioimagination.com>
 * http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
 */

void bqMakeLowpass(FloatBiquadCoeffs* c, float w0, float q)
{
    /* The cookbook says:
            b0 =  (1 - cos(w0))/2
            b1 =   1 - cos(w0)
            b2 =  (1 - cos(w0))/2
            a0 =   1 + alpha
            a1 =  -2*cos(w0)
            a2 =   1 - alpha
     */

    const float alpha = sinf(w0)/(2.0f*q);
    const float a0 = 1.0f + alpha;
    const float a0inv = 1.0 / a0;
    const float cosw0 = cosf(w0);
    c->gain = a0;
    c->a1 = -2.0f * cosw0 * a0inv;
    c->a2 = (1.0f - alpha) * a0inv;
    c->b1 = (1.0f - cosw0) * a0inv;
    c->b0 = c->b1 * 0.5f;
    c->b2 = c->b0;
}

void bqMakeBandpass(FloatBiquadCoeffs* c, float w0, float q)
{
    /* The cookbook says:
            b0 =   sin(w0)/2  =   Q*alpha
            b1 =   0
            b2 =  -sin(w0)/2  =  -Q*alpha
            a0 =   1 + alpha
            a1 =  -2*cos(w0)
            a2 =   1 - alpha
     */

    const float alpha = sinf(w0)/(2.0f*q);
    const float a0 = 1.0f + alpha;
    const float a0inv = 1.0 / a0;
    const float cosw0 = cosf(w0);
    c->gain = a0;
    c->a1 = -2.0f * cosw0 * a0inv;
    c->a2 = (1.0f - alpha) * a0inv;
    c->b0 = q * alpha * a0inv;
    c->b1 = 0;
    c->b2 = -c->b0;
}

void bqProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const FloatBiquadCoeffs* c, FloatBiquadState* state)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        float y[2];
        y[0] = c->gain * c->b0*in->s[s][0] +
                c->b1*state->X[0][0] + c->b2*state->X[1][0] -
                c->a1*state->Y[0][0] - c->a2*state->Y[1][0];
        y[1] = c->gain * c->b0*in->s[s][1] +
                c->b1*state->X[0][1] + c->b2*state->X[1][1] -
                c->a1*state->Y[0][1] - c->a2*state->Y[1][1];
        out->s[s][0] = y[0];
        out->s[s][1] = y[1];
        state->X[1][0] = state->X[0][0];
        state->X[0][0] = in->s[s][0];
        state->Y[1][0] = state->Y[0][0];
        state->Y[0][0] = y[0];
        state->X[1][1] = state->X[0][1];
        state->X[0][1] = in->s[s][1];
        state->Y[1][1] = state->Y[0][1];
        state->Y[0][1] = y[1];
    }
}
