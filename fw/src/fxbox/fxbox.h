#pragma once

#include "codec.h"

struct Params {
    float pedal;
    float knob1;
    float knob2;
    float knob3;
    float knob4;
};

typedef void(*FxProcess)(const AudioBuffer* restrict in,
        AudioBuffer* restrict out, const struct Params* params);

FxProcess runWahwah(void);
FxProcess runVibrato(void);
FxProcess runDelay(void);
FxProcess runPitcher(void);
