#include <string.h>

#include "codec.h"
#include "fxbox.h"
#include "platform.h"
#include "utils.h"
#include "waveshaper.h"

#define TAPS 4
#define DELAY_LINE_LENGTH (CODEC_SAMPLERATE/2)
static CodecIntSample delayline_l[DELAY_LINE_LENGTH];
static CodecIntSample delayline_r[DELAY_LINE_LENGTH];
static unsigned writepos;
static unsigned octaverPhase = 0;

struct Tap {
    float delay;
    float route[2][2]; // Levels to play L/R delay lines in L/R channel
};

static void process(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, const struct Params* params)
{
    setLed(LED_GREEN, true);

    const float input = params->pedal * 1.5f;
    const float feedback = params->knob3;
    const float confusion = params->knob1;
    const float octaveMix = 0.5f * confusion;

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        static float filteredLength = 0;
        filteredLength = 0.9999f * filteredLength + 0.0001f * params->knob4;
        const float length = filteredLength * DELAY_LINE_LENGTH;

        const struct Tap taps[TAPS] = {
                { length, { { 1.0f, 0.0f }, { 0.0f, 1.0f } } },
                { length / 2, { { 0.0f, -1.0f * confusion }, { 0.5f * confusion, 0.0f } } },
                { length / 3, { { 0.0f, 0.3f * confusion }, { -0.6f * confusion, 0.0f } } },
                { length / 4, { { 0.1f * confusion, -0.2f * confusion }, { -0.2f * confusion, 0.1f * confusion } } },
        };

        for (unsigned tap = 0; tap < TAPS; tap++) {
            if (taps[tap].delay) {
                float delayed[2] = { linterpolate(delayline_l, DELAY_LINE_LENGTH,
                        DELAY_LINE_LENGTH + writepos - taps[tap].delay),
                        linterpolate(delayline_r, DELAY_LINE_LENGTH,
                                DELAY_LINE_LENGTH + writepos - taps[tap].delay) };
                out->s[s][0] += taps[tap].route[0][0] * delayed[0] + taps[tap].route[0][1] * delayed[1];
                out->s[s][1] += taps[tap].route[1][0] * delayed[0] + taps[tap].route[1][1] * delayed[1];
            }
        }

        // FIXME: There is a discontinuity when octaverPhase wraps.
        out->s[s][0] += octaveMix * linterpolate(delayline_l, DELAY_LINE_LENGTH,
                DELAY_LINE_LENGTH + writepos - length + octaverPhase);
        out->s[s][1] += octaveMix * linterpolate(delayline_r, DELAY_LINE_LENGTH,
                DELAY_LINE_LENGTH + writepos - length + octaverPhase);

        delayline_l[writepos] = saturateSoft(input * in->s[s][0] + feedback * out->s[s][0]);
        delayline_r[writepos] = saturateSoft(input * in->s[s][1] + feedback * out->s[s][1]);

        octaverPhase = (octaverPhase + 1) % (unsigned)length;
        writepos = (writepos + 1) % DELAY_LINE_LENGTH;

        // Always feed through the input audio
        out->s[s][0] += in->s[s][0];
        out->s[s][1] += in->s[s][1];
    }

    setLed(LED_GREEN, false);
}

FxProcess runDelay(void)
{
    memset(delayline_l, 0, sizeof(delayline_l));
    memset(delayline_r, 0, sizeof(delayline_r));
    return process;
}
