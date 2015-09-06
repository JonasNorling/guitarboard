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

static inline void samplesToFloat(const AudioBuffer* restrict in,
        FloatAudioBuffer* restrict out)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        out->m[s] = in->m[s];
    }
}

static inline void floatToSamples(const FloatAudioBuffer* restrict in,
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

/**
 * Pick a non-integer position from a lookup table or delay line
 */
static inline float linterpolate(const CodecIntSample* table, unsigned tablelen, float pos)
{
    const unsigned intpos = (unsigned)pos;
    const float s0 = table[intpos % tablelen];
    const float s1 = table[(intpos + 1) % tablelen];
    const float frac = pos - intpos;
    return s0 * (1.0f - frac) + s1 * frac;
}

/**
 * Pick a non-integer position from a lookup table or delay line
 */
static inline float linterpolateFloat(const float* table, unsigned tablelen, float pos)
{
    const unsigned intpos = (unsigned)pos;
    const float s0 = table[intpos % tablelen];
    const float s1 = table[(intpos + 1) % tablelen];
    const float frac = pos - intpos;
    return s0 * (1.0f - frac) + s1 * frac;
}
