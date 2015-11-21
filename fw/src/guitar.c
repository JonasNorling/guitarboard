/*
 * This is the main file defining the software for a board embedded in a
 * guitar. Four parameter knobs and one three-position switch is used as control
 * inputs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "dsp/delay.h"
#include "dsp/vibrato.h"
#include "dsp/waveshaper.h"
#include "platform.h"
#include "utils.h"

enum Effects {
    EFFECT_NONE,
    EFFECT_VIBRATO,
    EFFECT_DELAY,
    EFFECT_QUIET,
    EFFECTS_COUNT
};

static enum Effects currentEffect = EFFECT_DELAY;
static VibratoState vibratoState;
static DelayState delayState;


static void feedthrough(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out)
{
    *out = *in;
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);

    FloatAudioBuffer fin;
    samplesToFloat(in, &fin);
    FloatAudioBuffer fout = { .m = { 0 } };

    const float knobs[4] = {
            RAMP_U16(knob(0), 0.0f, 1.0f),
            RAMP_U16(knob(1), 0.0f, 1.0f),
            RAMP_U16(knob(2), 0.0f, 1.0f),
            RAMP_U16(knob(3), 0.0f, 1.0f),
    };

    switch (currentEffect) {
    case EFFECT_QUIET:
        // Fade out whatever is still in the output buffers
        for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
            out->m[s] = out->m[s] >> 1;
        }
        break;
    case EFFECT_VIBRATO: {
        const VibratoParams params = {
                .speed = exp2f(RAMP(knobs[1], 0.0001f, 0.005f)) - 1.0f,
                .depth = knobs[0] * (VIBRATO_MAX_DEPTH-1),
                .phasediff = knobs[2] * M_PI/4
        };
        processVibrato(&fin, &fout, &vibratoState, &params);
        break;
    }
    case EFFECT_DELAY: {
        const DelayParams params = {
                .input = knobs[1],
                .confusion = knobs[0],
                .feedback = knobs[1],
                .octaveMix = 0.5f * knobs[0],
                .length = knobs[2]
        };
        processDelay(&fin, &fout, &delayState, &params);
        break;
    }
    default:
        feedthrough(&fin, &fout);
    }

    if (willClip(&fout)) {
        setLed(LED_RED, true);
    }
    else {
        setLed(LED_RED, false);
    }

    const float gain = knobs[3];
    const float gainExp = exp2f(6*gain);
    const float tubeMix = CLAMP(2*gain, 0.0f, 1.0f);
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        fout.m[s] *= gainExp;
        fout.m[s] = RAMP(tubeMix, saturateSoft(fout.m[s]), tubeSaturate(fout.m[s]));
    }

    floatToSamples(&fout, out);

    setLed(LED_GREEN, false);
}

static void idleCallback()
{
    enum Effects selected = button(4) + (button(5) << 1);
    if (currentEffect != selected) {
        // Switch effects via silence, trying to avoid a pop. While
        // we're looping here the audio processing will continue in
        // interrupt context.
        currentEffect = EFFECT_QUIET;
        for (unsigned i = 0; i < 100000; i++) __asm__("nop");
        currentEffect = selected;
    }
}

int main()
{
    const KnobConfig knobConfig[KNOB_COUNT] = {
            { .analog = true },
            { .analog = true },
            { .analog = true },
            { .analog = true },
            { .pullup = true },
            { .pullup = true },
    };

    platformInit(knobConfig);
    codedSetInVolume(5);
    codedSetOutVolume(-20);

    printf("Starting guitar board\n");

    platformRegisterIdleCallback(idleCallback);
    codecRegisterProcessFunction(process);

    initDelay(&delayState);
    initVibrato(&vibratoState);

    platformMainloop();

    return 0;
}
