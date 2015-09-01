#include <math.h>
#include <string.h>
#include "platform.h"
#include "codec.h"
#include "fxbox.h"

static const CodecIntSample TYP_AMPLITUDE=1000;

static CodecIntSample distort(CodecIntSample _x)
{
    // Clippy, tube-like distortion
    // References : Posted by Partice Tarrabia and Bram de Jong
    const float depth = 0.5;
    const float k = 2.0 * depth / (1.0 - depth);
    const float b = TYP_AMPLITUDE;
    const float x = _x / b;
    return b * (1 + k) * x / (1 + k * fabsf(x));
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_RED, true);

    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        out->m[s] = distort(in->m[s]);
    }

    setLed(LED_RED, false);
}

void runWahwah(void)
{
    codecRegisterProcessFunction(process);
}
