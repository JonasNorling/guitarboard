#pragma once

#include <math.h>
#include "utils.h"

static inline float tubeSaturate(float _x)
{
    // Clippy, tube-like distortion
    // References : Posted by Partice Tarrabia and Bram de Jong
    const float depth = 0.5;
    const float k = 2.0 * depth / (1.0 - depth);
    const float b = INT16_MAX;
    const float x = _x / b;
    const float ret = b * (1 + k) * x / (1 + k * fabsf(x));
    if (ret < INT16_MIN) return INT16_MIN;
    if (ret > INT16_MAX) return INT16_MAX;
    return ret;
}

static inline float saturateClip(float x)
{
    return CLAMP(x, INT16_MIN, INT16_MAX);
}

static inline float saturateSoft(float x)
{
    // x^3 polynomial
    const float a = 0.2;
    const float b = INT16_MAX;

    if (x > INT16_MAX) return INT16_MAX;
    if (x < INT16_MIN) return INT16_MIN;

    x = x / b;
    return b * ((1 + a) * x - a * x * x * x);
}
