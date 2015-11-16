#include <string.h>

#include "delay.h"
#include "utils.h"
#include "waveshaper.h"

#define TAPS 4

struct Tap {
    float delay;
    float route[2][2]; // Levels to play L/R delay lines in L/R channel
};

void processDelay(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, DelayState* st,
        const DelayParams* p)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        st->filteredLength = 0.9999f * st->filteredLength + 0.0001f * p->length;
        const float length = st->filteredLength * DELAY_LINELEN;

        const struct Tap taps[TAPS] = {
                { length,
                        { { 1.0f, 0.0f },
                        { 0.0f, 1.0f } } },
                { length / 2,
                        { { 0.0f, -1.0f * p->confusion },
                        { 0.5f * p->confusion, 0.0f } } },
                { length / 3,
                        { { 0.0f, 0.3f * p->confusion },
                        { -0.6f * p->confusion, 0.0f } } },
                { length / 4,
                        { { 0.1f * p->confusion, -0.2f * p->confusion },
                        { -0.2f * p->confusion, 0.1f * p->confusion } } },
        };

        for (unsigned tap = 0; tap < TAPS; tap++) {
            if (taps[tap].delay) {
                float delayed[2] = { linterpolate(st->delayline_l, DELAY_LINELEN,
                        DELAY_LINELEN + st->writepos - taps[tap].delay),
                        linterpolate(st->delayline_r, DELAY_LINELEN,
                                DELAY_LINELEN + st->writepos - taps[tap].delay) };
                out->s[s][0] += taps[tap].route[0][0] * delayed[0] +
                        taps[tap].route[0][1] * delayed[1];
                out->s[s][1] += taps[tap].route[1][0] * delayed[0] +
                        taps[tap].route[1][1] * delayed[1];
            }
        }

        // FIXME: There is a discontinuity when octaverPhase wraps.
        out->s[s][0] += p->octaveMix * linterpolate(st->delayline_l, DELAY_LINELEN,
                DELAY_LINELEN + st->writepos - length + st->octaverPhase);
        out->s[s][1] += p->octaveMix * linterpolate(st->delayline_r, DELAY_LINELEN,
                DELAY_LINELEN + st->writepos - length + st->octaverPhase);

        st->delayline_l[st->writepos] = saturateSoft(p->input * in->s[s][0] +
                p->feedback * out->s[s][0]);
        st->delayline_r[st->writepos] = saturateSoft(p->input * in->s[s][1] +
                p->feedback * out->s[s][1]);

        st->octaverPhase = (st->octaverPhase + 1) % (size_t)length;
        st->writepos = (st->writepos + 1) % DELAY_LINELEN;

        // Always feed through the input audio
        out->s[s][0] += in->s[s][0];
        out->s[s][1] += in->s[s][1];
    }
}

void initDelay(DelayState* state)
{
    memset(state, 0, sizeof(*state));
}
