#pragma once
#include <stdint.h>

#define CODEC_SAMPLERATE 48000
#define NYQUIST (CODEC_SAMPLERATE / 2)

// 64 samples = 1.33ms at 48kHz
#define CODEC_SAMPLES_PER_FRAME 64

typedef int16_t CodecIntSample;

typedef struct {
    union {
        CodecIntSample s[CODEC_SAMPLES_PER_FRAME][2] __attribute__((aligned(4)));
        CodecIntSample m[2*CODEC_SAMPLES_PER_FRAME] __attribute__((aligned(4)));
    };
} AudioBuffer;

typedef struct {
    union {
        float s[CODEC_SAMPLES_PER_FRAME][2];
        float m[2*CODEC_SAMPLES_PER_FRAME];
    };
} FloatAudioBuffer;

typedef void(*CodecProcess)(const AudioBuffer* restrict in, AudioBuffer* restrict out);

void codecRegisterProcessFunction(CodecProcess fn);
void codedSetInVolume(int vol);
void codedSetOutVolume(int voldB);
