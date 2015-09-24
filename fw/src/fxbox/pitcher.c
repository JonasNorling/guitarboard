/**
 * This is a naive time-domain pitch shifter effect.
 */

#include <string.h>

#include "codec.h"
#include "fxbox.h"
#include "platform.h"
#include "utils.h"
#include "waveshaper.h"

#define DELAY_LINE_LENGTH (CODEC_SAMPLERATE/25)
#define SCAN_LENGTH (DELAY_LINE_LENGTH - 2)
static CodecIntSample delayline_l[DELAY_LINE_LENGTH];
static CodecIntSample delayline_r[DELAY_LINE_LENGTH];
static unsigned writepos;
static float readphase[2];

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out,
        const struct Params* params)
{
    setLed(LED_GREEN, true);
    setLed(LED_RED, false);

    const float speed = RAMP(params->pedal, 0.0f, 1.0f);
    const float wet = RAMP(params->knob1, 0.0f, 1.0f);
    const float phasediff = RAMP(params->knob3, 0.0f, 0.02f);

    FloatAudioBuffer buf1;

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        readphase[0] += speed + phasediff;
        if (readphase[0] >= SCAN_LENGTH) {
            readphase[0] -= SCAN_LENGTH;
        }
        else if (readphase[0] < 0.0f) {
            readphase[0] += SCAN_LENGTH;
        }
        readphase[1] += speed - phasediff;
        if (readphase[1] >= SCAN_LENGTH) {
            readphase[1] -= SCAN_LENGTH;
        }
        else if (readphase[1] < 0.0f) {
            readphase[1] += SCAN_LENGTH;
        }

        // Mix two offsets in and out with a triangular window
        const float mix[2] = {
                fabs(readphase[0] - SCAN_LENGTH/2) / (SCAN_LENGTH/2),
                fabs(readphase[1] - SCAN_LENGTH/2) / (SCAN_LENGTH/2),
        };

        buf1.s[s][0] =
                RAMP(wet,
                        RAMP(mix[0],
                                linterpolate(delayline_l, DELAY_LINE_LENGTH, writepos + readphase[0]),
                                linterpolate(delayline_l, DELAY_LINE_LENGTH, writepos + readphase[0] + SCAN_LENGTH/2)),
                                in->s[s][0]);
        buf1.s[s][1] =
                RAMP(wet,
                        RAMP(mix[1],
                                linterpolate(delayline_r, DELAY_LINE_LENGTH, writepos + readphase[1]),
                                linterpolate(delayline_r, DELAY_LINE_LENGTH, writepos + readphase[1] + SCAN_LENGTH/2)),
                                in->s[s][1]);



        delayline_l[writepos] = in->s[s][0];
        delayline_r[writepos] = in->s[s][1];
        writepos = (writepos + 1) % DELAY_LINE_LENGTH;
    }

    if (willClip(&buf1)) {
        setLed(LED_RED, true);
    }

    floatToSamples(&buf1, out);
    setLed(LED_GREEN, false);
}

FxProcess runPitcher(void)
{
    memset(delayline_l, 0, sizeof(delayline_l));
    memset(delayline_r, 0, sizeof(delayline_r));
    return process;
}
