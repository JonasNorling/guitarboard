#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "codec.h"

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_RED, true);
    *out = *in;

    setLed(LED_GREEN, false);
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        const CodecIntSample max = in->s[s][0] > in->s[s][1] ? in->s[s][0] : in->s[s][1];
        if (abs(max) > 500) {
            setLed(LED_GREEN, true);
            break;
        }
    }

    setLed(LED_RED, false);
}

int main()
{
    platformInit(NULL);

    printf("Starting feedthrough example\n");

    codecRegisterProcessFunction(process);
    platformMainloop();

    return 0;
}
