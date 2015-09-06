#pragma once

/**
 * Convert a frequency to radians for making filters
 */
#define HZ2OMEGA(f) ((f) * (3.1416f / NYQUIST))

/**
 * Convert a 16-bit unsigned to a float value in the provided range, linearly.
 */
#define RAMP_U16(v, start, end) ((end * ((float)(v))/UINT16_MAX) + \
        (start * ((float)(UINT16_MAX-(v)))/UINT16_MAX))

#define CLAMP(v, min, max) (v) > (max) ? (max) : ((v) < (min) ? (min) : (v))

static void samplesToFloat(const AudioBuffer* restrict in,
        FloatAudioBuffer* restrict out)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        out->m[s] = in->m[s];
    }
}

static void floatToSamples(const FloatAudioBuffer* restrict in,
        AudioBuffer* restrict out)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        out->m[s] = in->m[s];
    }
}

/**
 * Return true if there are samples that can't be represented as a 16-bit
 * integer in the buffer.
 */
static bool willClip(const FloatAudioBuffer* restrict in)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        if (in->m[s] < INT16_MIN || in->m[s] > INT16_MAX) {
            return true;
        }
    }
    return false;
}
