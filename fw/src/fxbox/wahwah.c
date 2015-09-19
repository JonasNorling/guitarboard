#include <math.h>
#include <string.h>
#include "platform.h"
#include "biquad.h"
#include "codec.h"
#include "fxbox.h"
#include "utils.h"
#include "waveshaper.h"

static FloatBiquadCoeffs coeffs;
static FloatBiquadState bqstate;

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out,
        const struct Params* params)
{
    setLed(LED_GREEN, true);
    setLed(LED_RED, false);

    const float dist = params->knob2;
    // Set centre wah bandpass frequency to 200..800 Hz
    const float wah = RAMP(params->pedal, HZ2OMEGA(200), HZ2OMEGA(800));
    // Set bandpass Q to 1..32, on an exponential scale
    const float q = exp2f(RAMP(params->knob1, 0.0f, 5.0f));
    bqMakeBandpass(&coeffs, wah, q);

    FloatAudioBuffer buf1, buf2;

    samplesToFloat(in, &buf1);
    bqProcess(&buf1, &buf2, &coeffs, &bqstate);
    if (willClip(&buf2)) {
        setLed(LED_RED, true);
    }

    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        buf1.m[s] = RAMP(dist, saturateSoft(buf2.m[s]), tubeSaturate(buf2.m[s]));
    }

    floatToSamples(&buf1, out);
    setLed(LED_GREEN, false);
}

FxProcess runWahwah(void)
{
    bqMakeBandpass(&coeffs, 1.5f, 1.0f);
    return process;
}
