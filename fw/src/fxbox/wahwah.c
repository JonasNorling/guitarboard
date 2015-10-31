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

static void process(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, const struct Params* params)
{
    setLed(LED_GREEN, true);

    // Set centre wah bandpass frequency to 200..800 Hz
    const float wah = RAMP(params->pedal, HZ2OMEGA(200), HZ2OMEGA(800));
    // Set bandpass Q to 1..32, on an exponential scale
    const float q = exp2f(RAMP(params->knob1, 0.0f, 5.0f));
    bqMakeBandpass(&coeffs, wah, q);

    bqProcess(in, out, &coeffs, &bqstate);

    setLed(LED_GREEN, false);
}

FxProcess runWahwah(void)
{
    bqMakeBandpass(&coeffs, 1.5f, 1.0f);
    return process;
}
