#include <string.h>

#include "codec.h"
#include "fxbox.h"
#include "platform.h"
#include "utils.h"
#include "waveshaper.h"

#define PI 3.14159265358979323846

#define MAX_DEPTH 50
#define DELAY_LINE_LENGTH (MAX_DEPTH * 2)
static float delayline_l[DELAY_LINE_LENGTH];
static float delayline_r[DELAY_LINE_LENGTH];
static unsigned writepos;
static float phase;

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);
    setLed(LED_RED, false);

    const uint16_t dist = knob(2);
    const float speed = exp2f(RAMP_U16(knob(1), 0.0001f, 0.005f)) - 1.0f;
    const float depth = RAMP_U16(knob(0), 0.0f, (MAX_DEPTH-1));
    const float phasediff = RAMP_U16(knob(3), 0.0f, PI/4);

    FloatAudioBuffer buf1;

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        delayline_l[writepos] = in->s[s][0];
        delayline_r[writepos] = in->s[s][1];

        const float offset0 = depth * sinf(phase + phasediff);
        const float offset1 = depth * sinf(phase - phasediff);
        buf1.s[s][0] = linterpolateFloat(delayline_l, DELAY_LINE_LENGTH, offset0 +
                (writepos + (DELAY_LINE_LENGTH/2)));
        buf1.s[s][1] = linterpolateFloat(delayline_r, DELAY_LINE_LENGTH, offset1 +
                (writepos + (DELAY_LINE_LENGTH/2)));

        writepos = (writepos + 1) % DELAY_LINE_LENGTH;
        phase += speed;
        if (phase >= PI) {
            phase -= 2*PI;
        }
    }

    if (willClip(&buf1)) {
        setLed(LED_RED, true);
    }

    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        buf1.m[s] = RAMP_U16(dist, saturateSoft(buf1.m[s]),
                tubeSaturate(buf1.m[s]));
    }

    floatToSamples(&buf1, out);
    setLed(LED_GREEN, false);
}

void runVibrato(void)
{
    memset(delayline_l, 0, sizeof(delayline_l));
    memset(delayline_r, 0, sizeof(delayline_r));
    codecRegisterProcessFunction(process);
}
