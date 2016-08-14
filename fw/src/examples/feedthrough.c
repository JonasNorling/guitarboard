#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "codec.h"

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    static unsigned counter;

    counter++;
    setLed(LED_GREEN, counter & (1 << 8));

    *out = *in;

    setLed(LED_RED, false);
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        const CodecIntSample max = in->s[s][0] > in->s[s][1] ? in->s[s][0] : in->s[s][1];
        if (abs(max) > 500) {
            setLed(LED_RED, true);
            break;
        }
    }
}

int main()
{
    platformInit(NULL);

    printf("Starting feedthrough example\n");

    codecRegisterProcessFunction(process);
    platformMainloop();

    return 0;
}
