#include "fxbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "codec.h"

enum Effects {
    EFFECT_WAHWAH,
    EFFECT_VIBRATO,
    EFFECT_DELAY,
    EFFECTS_COUNT
};

static enum Effects currentEffect;

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    // Fade out whatever is still in the output buffers
    (void)in;
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = out->s[s][0] >> 1;
        out->s[s][1] = out->s[s][1] >> 1;
    }
}

static void setEffect(enum Effects fx)
{
    codecRegisterProcessFunction(process);
    for (unsigned i = 0; i < 100000; i++) __asm__("nop");
    switch (fx) {
    case EFFECT_WAHWAH:
        runWahwah();
        break;
    case EFFECT_VIBRATO:
        runVibrato();
        break;
    case EFFECT_DELAY:
        runDelay();
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
        if (fxSelector > fx * (UINT16_MAX/EFFECTS_COUNT) + 1000 &&
                fxSelector < (fx + 1) * (UINT16_MAX/EFFECTS_COUNT) - 1000) {
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

    runWahwah();
    currentEffect = EFFECT_WAHWAH;

    platformMainloop();

    return 0;
}
