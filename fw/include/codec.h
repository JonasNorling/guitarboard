#pragma once
#include <stdint.h>

#define CODEC_SAMPLERATE 48000

// 64 samples = 1.33ms at 48kHz
#define CODEC_SAMPLES_PER_FRAME 64

typedef int16_t CodecIntSample;

typedef struct {
  CodecIntSample s[CODEC_SAMPLES_PER_FRAME][2] __attribute__((aligned(4)));
} AudioBuffer;

typedef void(*CodecProcess)(const AudioBuffer* restrict in, AudioBuffer* restrict out);

void codecRegisterProcessFunction(CodecProcess fn);
