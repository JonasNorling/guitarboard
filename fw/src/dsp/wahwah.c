#include <math.h>
#include <string.h>

#include "wahwah.h"
#include "codec.h"
#include "utils.h"
#include "waveshaper.h"

void processWahwah(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out, WahwahState* state,
        const WahwahParams* params)
{
    // Set centre wah bandpass frequency to 200..800 Hz
    const float wah = RAMP(params->wah, HZ2OMEGA(200), HZ2OMEGA(800));
    // Set bandpass Q to 1..32, on an exponential scale
    const float q = exp2f(RAMP(params->q, 0.0f, 5.0f));

    FloatBiquadCoeffs coeffs;
    bqMakeBandpass(&coeffs, wah, q);
    bqProcess(in, out, &coeffs, &state->bqstate);
}

void initWahwah(WahwahState* state)
{
    memset(state, 0, sizeof(*state));
}
