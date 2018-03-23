#include <stdio.h>
#include <string.h>

#include "codec.h"
#include "platform.h"
#include "utils.h"

#include "wt.h"

struct wtOscCtx {
    float hz;
    float phaseAccumulator;
    const float* subtable;
};

static float interpolateTable(const float* table, int tablelen, float pos)
{
    const unsigned intpos = (unsigned)pos;
    const float s0 = table[intpos % tablelen];
    const float s1 = table[(intpos + 1) % tablelen];
    const float frac = pos - intpos;
    return s0 * (1.0f - frac) + s1 * frac;
}

static void wtOscProcess(struct wtOscCtx* ctx, float out[CODEC_SAMPLES_PER_FRAME])
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out[s] = interpolateTable(ctx->subtable, WT_LENGTH, ctx->phaseAccumulator);
        ctx->phaseAccumulator += ctx->hz * ((float)WT_LENGTH / CODEC_SAMPLERATE);
        if (ctx->phaseAccumulator >= WT_LENGTH) ctx->phaseAccumulator -= WT_LENGTH;
    }
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);
    (void)in;

    static struct wtOscCtx wtOscCtx[2] = {
        { 437, 0, wtSaw[0] },
        { 443, 0, wtSaw[0] }
    };

    static float bufs[2][CODEC_SAMPLES_PER_FRAME];

    wtOscProcess(&wtOscCtx[0], bufs[0]);
    wtOscProcess(&wtOscCtx[1], bufs[1]);

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = 0x8000 * bufs[0][s];
        out->s[s][1] = 0x8000 * bufs[1][s];
    }
    setLed(LED_GREEN, false);
}

int main()
{
    platformInit(NULL);
    codedSetOutVolume(-30);

    printf("Starting sawsynth\n");

    codecRegisterProcessFunction(process);

    platformMainloop();

    return 0;
}
