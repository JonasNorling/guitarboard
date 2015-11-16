/**
 * This is a naive time-domain pitch shifter effect.
 */

#include <math.h>
#include <string.h>

#include "codec.h"
#include "pitcher.h"
#include "utils.h"

void processPitcher(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, PitcherState* st,
        const PitcherParams* p)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        st->readphase[0] += p->speed + p->phasediff;
        if (st->readphase[0] >= SCAN_LENGTH) {
            st->readphase[0] -= SCAN_LENGTH;
        }
        else if (st->readphase[0] < 0.0f) {
            st->readphase[0] += SCAN_LENGTH;
        }
        st->readphase[1] += p->speed - p->phasediff;
        if (st->readphase[1] >= SCAN_LENGTH) {
            st->readphase[1] -= SCAN_LENGTH;
        }
        else if (st->readphase[1] < 0.0f) {
            st->readphase[1] += SCAN_LENGTH;
        }

        // Mix two offsets in and out with a triangular window
        const float mix[2] = {
                fabs(st->readphase[0] - SCAN_LENGTH/2) / (SCAN_LENGTH/2),
                fabs(st->readphase[1] - SCAN_LENGTH/2) / (SCAN_LENGTH/2),
        };

        out->s[s][0] =
                RAMP(p->wet,
                        RAMP(mix[0],
                                linterpolate(st->delayline_l, DELAY_LINE_LENGTH, st->writepos + st->readphase[0]),
                                linterpolate(st->delayline_l, DELAY_LINE_LENGTH, st->writepos + st->readphase[0] + SCAN_LENGTH/2)),
                                in->s[s][0]);
        out->s[s][1] =
                RAMP(p->wet,
                        RAMP(mix[1],
                                linterpolate(st->delayline_r, DELAY_LINE_LENGTH, st->writepos + st->readphase[1]),
                                linterpolate(st->delayline_r, DELAY_LINE_LENGTH, st->writepos + st->readphase[1] + SCAN_LENGTH/2)),
                                in->s[s][1]);



        st->delayline_l[st->writepos] = in->s[s][0];
        st->delayline_r[st->writepos] = in->s[s][1];
        st->writepos = (st->writepos + 1) % DELAY_LINE_LENGTH;
    }
}

void initPitcher(PitcherState* state)
{
    memset(state, 0, sizeof(*state));
}
