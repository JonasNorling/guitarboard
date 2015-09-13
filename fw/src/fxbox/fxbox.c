#include "fxbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "codec.h"

enum Effects {
    EFFECT_WAHWAH,
    EFFECT_VIBRATO,
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

static void idleCallback()
{
    const uint16_t fxSelector = knob(5);
    if (fxSelector < 30000 && currentEffect != EFFECT_WAHWAH) {
        codecRegisterProcessFunction(process);
        for (unsigned i = 0; i < 100000; i++) __asm__("nop");
        runWahwah();
        currentEffect = EFFECT_WAHWAH;
    }
    else if (fxSelector > 35000 && currentEffect != EFFECT_VIBRATO) {
        codecRegisterProcessFunction(process);
        for (unsigned i = 0; i < 100000; i++) __asm__("nop");
        runVibrato();
        currentEffect = EFFECT_VIBRATO;
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
