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

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);
    setLed(LED_RED, false);

    const uint16_t wah = UINT16_MAX - knob(0); // My pedal is backwards
    const uint16_t q = knob(1);
    const uint16_t dist = knob(2);
    // Set centre wah bandpass frequency to 200..800 Hz
    const float fwah = RAMP_U16(wah, HZ2OMEGA(200), HZ2OMEGA(800));
    // Set bandpass Q to 1..32, on an exponential scale
    const float fq = exp2f(RAMP_U16(q, 0.0f, 5.0f));
    bqMakeBandpass(&coeffs, fwah, fq);
    coeffs.gain *= RAMP_U16(wah, 1.0f, 2.0f);

    FloatAudioBuffer buf1, buf2;

    samplesToFloat(in, &buf1);
    bqProcess(&buf1, &buf2, &coeffs, &bqstate);
    if (willClip(&buf2)) {
        setLed(LED_RED, true);
    }

    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        buf1.m[s] = RAMP_U16(dist, saturateSoft(buf2.m[s]),
                tubeSaturate(buf2.m[s]));
    }

    floatToSamples(&buf1, out);
    setLed(LED_GREEN, false);
}

void runWahwah(void)
{
    bqMakeBandpass(&coeffs, 1.5f, 1.0f);
    codecRegisterProcessFunction(process);
}
