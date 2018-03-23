#include <stdio.h>
#include <string.h>
#include <math.h>

#include "codec.h"
#include "platform.h"
#include "utils.h"
#include "dsp/waveshaper.h"
#include "dsp/biquad.h"

#include "wt.h"

#define NOTE2HZ(note) (exp2f(((float)note - 69) / 12.0) * 440)

struct wtOscCtx {
    float hz;
    float phaseAccumulator;
    const float* subtable;
};

struct voice {
    bool active;
    uint8_t note;
    uint8_t velocity;
    struct wtOscCtx oscs[3];
    FloatBiquadCoeffs filterCoeffs;
    FloatBiquadState filterState;
};

struct sequencerEvent {
    uint8_t at;
    uint8_t note;
    uint8_t velocity;
};

static const struct sequencerEvent events[] = {
    { 0x00, 60, 120 },
    { 0x08, 60, 0 },
    { 0x10, 67, 100 },
    { 0x18, 67, 0 },
    { 0x20, 70, 100 },
    { 0x2d, 70, 0 },
    { 0x30, 72, 80 },
    { 0x38, 72, 0 },
    { 0x40, 58, 120 },
    { 0x48, 58, 0 },
    { 0x50, 62, 100 },
    { 0x58, 62, 0 },
    { 0x60, 65, 100 },
    { 0x6c, 65, 0 },
    { 0x70, 63, 80 },
    { 0x78, 63, 0 },
};

static struct voice voices[8];


static void wtOscProcess(struct wtOscCtx* ctx, float out[CODEC_SAMPLES_PER_FRAME])
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out[s] += linterpolateFloat(ctx->subtable, WT_LENGTH, ctx->phaseAccumulator);
        ctx->phaseAccumulator += ctx->hz * ((float)WT_LENGTH / CODEC_SAMPLERATE);
        if (ctx->phaseAccumulator >= WT_LENGTH) ctx->phaseAccumulator -= WT_LENGTH;
    }
}

static void noteOn(uint8_t note, uint8_t velocity)
{
    struct voice* voice = NULL;
    for (size_t i = 0; i < ARRAY_SIZE(voices); i++) {
        if (!voices[i].active) {
            voice = &voices[i];
        }
    }
    if (!voice) {
        return;
    }
    
    memset(voice, 0, sizeof(*voice));
    voice->note = note;
    voice->velocity = velocity;
    voice->oscs[0].hz = NOTE2HZ(note) * (1 + 24/12000.0);
    voice->oscs[0].subtable = wtSaw[0];
    voice->oscs[1].hz = NOTE2HZ(note) * (1 - 24/12000.0);
    voice->oscs[1].subtable = wtSaw[0];
    voice->oscs[2].hz = NOTE2HZ(note);
    voice->oscs[2].subtable = wtSaw[0];
    bqMakeLowpass(&voice->filterCoeffs, HZ2OMEGA(40 + velocity * 80), 0.3);
    voice->active = true;
}

static void noteOff(uint8_t note)
{
    for (size_t i = 0; i < ARRAY_SIZE(voices); i++) {
        if (voices[i].active && voices[i].note == note) {
            voices[i].active = false;
        }
    }
}

static void stepSequencer()
{
    static unsigned frameCount;
    frameCount++;

    if (frameCount % 8) {
        return;
    }

    static uint8_t substep;
    substep = (substep + 1) % 0x80;

    for (size_t i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i].at == substep) {
            if (events[i].velocity) {
                noteOn(events[i].note, events[i].velocity);
            }
            else {
                noteOff(events[i].note);
            }
        }
    }
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);
    (void)in;

    memset(out, 0, sizeof(AudioBuffer));

    for (size_t i = 0; i < ARRAY_SIZE(voices); i++) {
        struct voice* voice = &voices[i];
        if (!voice->active) {
            continue;
        }

        float buf1[CODEC_SAMPLES_PER_FRAME] = {};

        for (size_t o = 0; o < ARRAY_SIZE(voice->oscs); o++) {
            wtOscProcess(&voice->oscs[o], buf1);
        }

        for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
            //buf1[s] = tubeSaturate(buf1[s]);
        }

        FloatAudioBuffer filterin, filterout;
        for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
            filterin.s[s][0] = buf1[s];
            filterin.s[s][1] = buf1[s];
        }
        bqProcess(&filterin, &filterout, &voice->filterCoeffs, &voice->filterState);

        for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
            out->s[s][0] += 0x1000 * filterout.s[s][0];
            out->s[s][1] += 0x1000 * filterout.s[s][1];
        }
    }

    setLed(LED_GREEN, false);

    stepSequencer();
}

int main()
{
    platformInit(NULL);
    codedSetOutVolume(-40);

    printf("Starting sawsynth\n");

    codecRegisterProcessFunction(process);

    platformMainloop();

    return 0;
}
