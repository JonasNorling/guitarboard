#pragma once
#include <stdint.h>

#define CODEC_SAMPLES_PER_FRAME 16

typedef uint16_t CodecIntSample;

typedef struct {
    CodecIntSample s[2][CODEC_SAMPLES_PER_FRAME];
} AudioBuffer;

typedef void(*CodecProcess)(const AudioBuffer* restrict in, AudioBuffer* restrict out);

void codecRegisterProcessFunction(CodecProcess fn);
