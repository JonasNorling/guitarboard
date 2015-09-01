#include "fxbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "codec.h"

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    // Fade out whatever is still in the output buffers
    (void)in;
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = out->s[s][0] >> 1;
        out->s[s][1] = out->s[s][1] >> 1;
    }
}

int main()
{
    platformInit();

    printf("Starting fxbox\n");

    codecRegisterProcessFunction(process);
    runWahwah();
    platformMainloop();

    return 0;
}
