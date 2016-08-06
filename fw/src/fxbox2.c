/*
 * This is the main file defining the software that goes into an effects box
 * with 4 switches and 5 knobs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "dsp/biquad.h"
#include "dsp/delay.h"
#include "dsp/vibrato.h"
#include "dsp/waveshaper.h"
#include "platform.h"
#include "utils.h"

static VibratoState vibratoState;
static FloatBiquadState bqstate;
static DelayState delayState;

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);

    FloatAudioBuffer b1, b2, b3;
    samplesToFloat(in, &b1);
    FloatAudioBuffer* outBuf = &b1;

    const float knobs[6] = {
            RAMP_U16(knob(0), 0.0f, 16.15f),
            RAMP_U16(knob(1), 0.0f, 1.0f),
            RAMP_U16(knob(2), 0.0f, 1.0f),
            RAMP_U16(knob(3), 0.0f, 1.0f),
            RAMP_U16(knob(4), 0.0f, 1.0f),
            RAMP_U16(knob(5), 0.0f, 1.0f)
    };

    uint8_t switches = ~(unsigned)roundf(knobs[0]) & 0x0f;

    const VibratoParams vParams = {
            .speed = exp2f(RAMP(knobs[1], 0.0001f, 0.005f)) - 1.0f,
            .depth = knobs[2] * (VIBRATO_MAX_DEPTH-1),
            .phasediff = knobs[3] * M_PI/4
    };
    processVibrato(&b1, &b2, &vibratoState, &vParams);
    outBuf = (switches & 0x01) ? &b2 : &b1;

    FloatBiquadCoeffs coeffs;
    bqMakeBandpass(&coeffs, RAMP(knobs[4], HZ2OMEGA(20), HZ2OMEGA(800)),
            exp2f(RAMP(knobs[1], 0.0f, 5.0f)));
    bqProcess(outBuf, &b3, &coeffs, &bqstate);
    outBuf = (switches & 0x02) ? &b3 : outBuf;

    const DelayParams dParams = {
            .input = knobs[1],
            .confusion = 0.3,
            .feedback = knobs[2],
            .octaveMix = 0.5f * knobs[4],
            .length = knobs[3]
    };
    FloatAudioBuffer b4 = { .m = { } };
    processDelay(outBuf, &b4, &delayState, &dParams);
    outBuf = (switches & 0x04) ? &b4 : outBuf;

    if (switches &0x08) {
        // Output the difference between channels
        for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
            float a = outBuf->s[s][0];
            float b = outBuf->s[s][1];
            outBuf->s[s][0] = a - b;
            outBuf->s[s][1] = b - a;
        }
    }

    setLed(LED_RED, willClip(outBuf));

    const float gain = knobs[5];
    const float gainExp = exp2f(6*gain);
    const float tubeMix = CLAMP(2*gain, 0.0f, 1.0f);
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        outBuf->m[s] *= gainExp;
        outBuf->m[s] = RAMP(tubeMix, saturateSoft(outBuf->m[s]), tubeSaturate(outBuf->m[s]));
    }

    floatToSamples(outBuf, out);

    setLed(LED_GREEN, false);
}

static void idleCallback()
{
}

int main()
{
    platformInit(NULL);
    codedSetInVolume(8);
    codedSetOutVolume(-10);

    printf("Starting fxbox 2\n");

    platformRegisterIdleCallback(idleCallback);
    codecRegisterProcessFunction(process);

    initVibrato(&vibratoState);
    initDelay(&delayState);

    platformMainloop();

    return 0;
}
