#include <string.h>

#include "vibrato.h"
#include "codec.h"
#include "utils.h"
#include "waveshaper.h"

void processVibrato(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, VibratoState* st,
        const VibratoParams* p)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        st->delayline_l[st->writepos] = in->s[s][0];
        st->delayline_r[st->writepos] = in->s[s][1];

        const float offset0 = p->depth * sinf(st->phase + p->phasediff);
        const float offset1 = p->depth * sinf(st->phase - p->phasediff);
        out->s[s][0] = linterpolateFloat(st->delayline_l, VIBRATO_LINELEN, offset0 +
                (st->writepos + (VIBRATO_LINELEN/2)));
        out->s[s][1] = linterpolateFloat(st->delayline_r, VIBRATO_LINELEN, offset1 +
                (st->writepos + (VIBRATO_LINELEN/2)));

        st->writepos = (st->writepos + 1) % VIBRATO_LINELEN;
        st->phase += p->speed;
        if (st->phase >= M_PI) {
            st->phase -= 2*M_PI;
        }
    }
}

void initVibrato(VibratoState* state)
{
    memset(state, 0, sizeof(*state));
}
