#include <math.h>
#include <stdio.h>
#include "platform.h"
#include "codec.h"

#define PI 3.14159265358979323846

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    (void)in;

    const float f[2] = { 261.63, 329.63 };
    static float pos[2];

    // About 440 Hz at 48kHz sample rate, full amplitude
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = 30000 * sinf(pos[0]);
        out->s[s][1] = 30000 * sinf(pos[1]);

        pos[0] += 2*PI * f[0] / CODEC_SAMPLERATE;
        if (pos[0] > 2*PI) {
            pos[0] -= 2*PI;
        }
        pos[1] += 2*PI * f[1] / CODEC_SAMPLERATE;
        if (pos[1] > 2*PI) {
            pos[1] -= 2*PI;
        }
    }
}

int main()
{
    platformInit();

    printf("Starting sinewave example\n");

    codecRegisterProcessFunction(process);
    platformMainloop();

    return 0;
}
