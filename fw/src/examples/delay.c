#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "codec.h"

#define T_MS 500
#define N_SAMPLES (CODEC_SAMPLERATE / 1000 * T_MS)
static const unsigned delay = N_SAMPLES-1;
static CodecIntSample line[N_SAMPLES][2];
static unsigned writepos = 0;

static const float mix = 0.75;

static float lowpass(float x, float* context)
{
    const float a = 0.2;
    *context = a * x + (1.0f-a) * *context;
    return *context;
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_RED, true);

    static float lpctx[2];

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        float vl = mix * line[(N_SAMPLES + writepos - delay) % N_SAMPLES][0] + (1.0-mix) * in->s[s][0];
        float vr = mix * line[(N_SAMPLES + writepos - delay) % N_SAMPLES][1] + (1.0-mix) * in->s[s][1];

        out->s[s][0] = vl;
        out->s[s][1] = vr;

        // Write back to delay line, pingponging, filtering
        vl = lowpass(vl, &lpctx[1]);
        vr = lowpass(vr, &lpctx[0]);
        line[writepos][0] = 0.85 * vr + 0.15 * vl;
        line[writepos][1] = 0.85 * vl + 0.15* vr;

        writepos = (writepos + 1) % N_SAMPLES;
    }

    setLed(LED_GREEN, false);
    if (abs(lpctx[0]) > 100) {
        setLed(LED_GREEN, true);
    }

    setLed(LED_RED, false);
}

int main()
{
    platformInit();

    printf("Starting delay example\n");

    codecRegisterProcessFunction(process);
    platformMainloop();

    return 0;
}
