/*
 * This is the main file defining the software that goes into an effects box
 * built in a wah-wah pedal case. It is controlled by one mode selection knob,
 * four parameter knobs and the pedal position.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include "dsp/delay.h"
#include "dsp/pitcher.h"
#include "dsp/vibrato.h"
#include "dsp/wahwah.h"
#include "dsp/waveshaper.h"
#include "platform.h"
#include "utils.h"

enum Effects {
    EFFECT_QUIET,
    EFFECT_WAHWAH,
    EFFECT_VIBRATO,
    EFFECT_DELAY,
    EFFECT_PITCHER,
    EFFECT_NONE2,
    EFFECT_NONE3,
    EFFECTS_COUNT
};

static enum Effects currentEffect = EFFECTS_COUNT;
static WahwahState wahwahState;
static VibratoState vibratoState;
static DelayState delayState;
static PitcherState pitcherState;


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

    const float knobs[5] = {
            // Foot pedal: toe around 1800->1.0f, heel around 57000->0.0f
            CLAMP(RAMP_U16(knob(0), 1.033f, -0.155f), 0.0f, 1.0f),
            RAMP_U16(knob(1), 1.0f, 0.0f),
            RAMP_U16(knob(2), 1.0f, 0.0f),
            RAMP_U16(knob(3), 1.0f, 0.0f),
            RAMP_U16(knob(4), 1.0f, 0.0f)
    };

    switch (currentEffect) {
    case EFFECT_QUIET:
        // Fade out whatever is still in the output buffers
        for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
            out->m[s] = out->m[s] >> 1;
        }
        break;

    case EFFECT_WAHWAH: {
        const WahwahParams params = {
                .wah = knobs[0],
                .q = knobs[1]
        };
        processWahwah(&fin, &fout, &wahwahState, &params);
        break;
    }
    case EFFECT_VIBRATO: {
        const VibratoParams params = {
                .speed = exp2f(RAMP(knobs[1], 0.0001f, 0.005f)) - 1.0f,
                .depth = knobs[0] * (VIBRATO_MAX_DEPTH-1),
                .phasediff = knobs[3] * M_PI/4
        };
        processVibrato(&fin, &fout, &vibratoState, &params);
        break;
    }
    case EFFECT_DELAY: {
        const DelayParams params = {
                .input = knobs[0],
                .confusion = knobs[1],
                .feedback = knobs[3],
                .octaveMix = 0.5f * knobs[1],
                .length = knobs[4]
        };
        processDelay(&fin, &fout, &delayState, &params);
        break;
    }
    case EFFECT_PITCHER: {
        const PitcherParams params = {
                .speed = knobs[0],
                .wet = knobs[1],
                .phasediff = 0.02f * knobs[3]
        };
        processPitcher(&fin, &fout, &pitcherState, &params);
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

    const float gain = knobs[2];
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
    // Switch the active effect if the selector knob has been turned, with some
    // hysteresis.
    const uint16_t fxSelector = knob(5);
    for (enum Effects fx = 0; fx < EFFECTS_COUNT; fx++) {
        if (fxSelector >= fx * (UINT16_MAX/EFFECTS_COUNT) &&
                fxSelector <= (fx + 1) * (UINT16_MAX/EFFECTS_COUNT)) {
            if (currentEffect != fx) {
                // Switch effects via silence, trying to avoid a pop. While
                // we're looping here the audio processing will continue in
                // interrupt context.
                currentEffect = EFFECT_QUIET;
                for (unsigned i = 0; i < 100000; i++) __asm__("nop");
                currentEffect = fx;
            }
            break;
        }
    }
}

int main()
{
    platformInit(NULL);

    printf("Starting fxbox\n");

    platformRegisterIdleCallback(idleCallback);
    codecRegisterProcessFunction(process);

    initDelay(&delayState);
    initVibrato(&vibratoState);
    initWahwah(&wahwahState);
    initPitcher(&pitcherState);

    platformMainloop();

    return 0;
}
