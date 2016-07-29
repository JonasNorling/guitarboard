#include <math.h>
#include <stdio.h>
#include "platform.h"
#include "codec.h"
#include <tools/kiss_fftr.h>

#define PI 3.14159265358979323846

#define N 2048
static kiss_fftr_cfg fftCfg;
static kiss_fft_scalar fftBuffer[2][N];
static unsigned currentWriteBuffer;
static unsigned fftInPtr;
static bool newFftBufferAvailable;

float signalW[2] = { 440.0 / CODEC_SAMPLERATE, 0 };

static float normPower(kiss_fft_cpx s)
{
    const float scale = N*N/16;
    return (s.r*s.r + s.i*s.i) / scale;
}

static float dB(float n)
{
    return 10 * log10(n);
}

/**
 * Runs in interrupt context. Copy input data to the FFT buffer. Play a
 * test signal.
 */
static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        fftBuffer[currentWriteBuffer][fftInPtr] = in->s[s][0] / 65536.0f;
        fftInPtr++;
    }

    if (fftInPtr >= N) {
        fftInPtr = 0;
        currentWriteBuffer = (currentWriteBuffer + 1) % 2;
        newFftBufferAvailable = true;
    }

    static float pos[2];

    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = 32767 * sinf(pos[0]);
        out->s[s][1] = 32767 * sinf(pos[1]);

        pos[0] += 2*PI * signalW[0];
        if (pos[0] > 2*PI) {
            pos[0] -= 2*PI;
        }
        pos[1] += 2*PI * signalW[1];
        if (pos[1] > 2*PI) {
            pos[1] -= 2*PI;
        }
    }
}

/**
 * Runs in the main loop, when the CPU is not sleeping.
 * Do FFT, chew on data.
 */
static void handleData()
{
    if (!newFftBufferAvailable) {
        return;
    }
    setLed(LED_GREEN, true);

    newFftBufferAvailable = false;
    const unsigned bufIdx = (currentWriteBuffer + 1) % 2;

    // Apply a Hann window. The function includes a correction factor
    // to remove the attenuation in the window.
    for (unsigned n = 0; n < N; n++) {
        fftBuffer[bufIdx][n] *= 1-cosf((2*PI*n)/(N-1));
    }

    static kiss_fft_cpx transform[N];
    kiss_fftr(fftCfg, fftBuffer[bufIdx], transform);

    float fundamentalBin = signalW[0] * N;
    double harm[4] = {};
    for (int i = -N/256; i <= N/256; i++) {
        harm[0] += normPower(transform[(int)fundamentalBin + i]);
        harm[1] += normPower(transform[(int)(2*fundamentalBin) + i]);
        harm[2] += normPower(transform[(int)(3*fundamentalBin) + i]);
        harm[3] += normPower(transform[(int)(4*fundamentalBin) + i]);
    }

    float thd = 1000000 * sqrtf((harm[1] + harm[2] + harm[3]) / harm[0]);
    printf("DC: %4d, Peaks: %3d %3d %3d %3d THD=%dppm\n", (int)dB(normPower(transform[0])),
            (int)dB(harm[0]), (int)dB(harm[1]), (int)dB(harm[2]), (int)dB(harm[3]),
            (int)thd);
    setLed(LED_GREEN, false);
}

int main()
{
    platformInit(NULL);

    printf("Starting test\n");

    fftCfg = kiss_fftr_alloc(N, false, NULL, NULL);

    codedSetOutVolume(0);
    codecRegisterProcessFunction(process);
    platformRegisterIdleCallback(handleData);
    platformMainloop();

    return 0;
}
