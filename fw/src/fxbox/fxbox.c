#include "fxbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "codec.h"
#include "utils.h"

enum Effects {
    EFFECT_WAHWAH,
    EFFECT_VIBRATO,
    EFFECT_DELAY,
    EFFECT_NONE1,
    EFFECT_NONE2,
    EFFECT_NONE3,
    EFFECTS_COUNT
};

static enum Effects currentEffect = EFFECTS_COUNT;
static FxProcess currentEffectFn;

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    struct Params params = {
            // Foot pedal: toe around 1800->1.0f, heel around 57000->0.0f
            CLAMP(RAMP_U16(knob(0), 1.033f, -0.155f), 0.0f, 1.0f),
            RAMP_U16(knob(1), 1.0f, 0.0f),
            RAMP_U16(knob(2), 1.0f, 0.0f),
            RAMP_U16(knob(3), 1.0f, 0.0f),
            RAMP_U16(knob(4), 1.0f, 0.0f),
    };
    if (currentEffectFn) {
        currentEffectFn(in, out, &params);
        return;
    }

    // Fade out whatever is still in the output buffers
    (void)in;
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = out->s[s][0] >> 1;
        out->s[s][1] = out->s[s][1] >> 1;
    }
}

static void setEffect(enum Effects fx)
{
    currentEffectFn = NULL;
    for (unsigned i = 0; i < 100000; i++) __asm__("nop");
    switch (fx) {
    case EFFECT_WAHWAH:
        currentEffectFn = runWahwah();
        break;
    case EFFECT_VIBRATO:
        currentEffectFn = runVibrato();
        break;
    case EFFECT_DELAY:
        currentEffectFn = runDelay();
        break;
    default:
        break;
    }
    currentEffect = fx;
}

static void idleCallback()
{
    const uint16_t fxSelector = knob(5);
    for (enum Effects fx = 0; fx < EFFECTS_COUNT; fx++) {
        if (fxSelector >= fx * (UINT16_MAX/EFFECTS_COUNT) &&
                fxSelector <= (fx + 1) * (UINT16_MAX/EFFECTS_COUNT)) {
            if (currentEffect != fx) {
                setEffect(fx);
            }
        }
    }
}

int main()
{
    platformInit();

    printf("Starting fxbox\n");

    platformRegisterIdleCallback(idleCallback);
    codecRegisterProcessFunction(process);

    platformMainloop();

    return 0;
}
