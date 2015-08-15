#include <stdio.h>
#include "platform.h"
#include "codec.h"

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    *out = *in;
}

int main()
{
    platformInit();

    printf("Starting feedthrough example\n");

    codecRegisterProcessFunction(process);
    platformMainloop();

    return 0;
}
