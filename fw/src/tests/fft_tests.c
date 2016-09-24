#define _POSIX_C_SOURCE 199309L
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "platform.h"
#include "codec.h"
#include "window.h"
#include <tools/kiss_fftr.h>

#ifndef HOST
#include <libopencmsis/core_cm3.h>
#endif

#define PI 3.14159265358979323846

#define N 2048
static kiss_fftr_cfg fftCfg;
static kiss_fft_scalar fftBuffer[2][2][N];
static kiss_fft_cpx transform[2][N];
static unsigned currentWriteBuffer;
static unsigned fftInPtr;
static volatile bool newFftBufferAvailable;

float signalW[2] = { 440.0 / CODEC_SAMPLERATE, 0 };

struct ThdResult {
    float fundamental; //< Power of fundamental (not in dB)
    float harmonics;  //< Sum of power of harmonics (not in dB)
    float thd; //< Total harmonic distortion
    float dc; //< DC power (not in dB)
    float other; //< Sum of all other frequency bins (not in dB)
};


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
        fftBuffer[currentWriteBuffer][0][fftInPtr] = in->s[s][0] / 65536.0f;
        fftBuffer[currentWriteBuffer][1][fftInPtr] = in->s[s][1] / 65536.0f;
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

static void waitForData()
{
    while (!newFftBufferAvailable) {
#ifdef HOST
        struct timespec t = { .tv_nsec = 1e6 };
        nanosleep(&t, NULL);
#else
        __WFI();
#endif
    }
    newFftBufferAvailable = false;
}

/**
 * Runs in the main loop, when the CPU is not sleeping.
 * Do FFT, chew on data.
 */
static void doFft()
{
    waitForData();
    kiss_fft_scalar* in[2] = {
            fftBuffer[(currentWriteBuffer + 1) % 2][0],
            fftBuffer[(currentWriteBuffer + 1) % 2][1]
    };

    setLed(LED_GREEN, true);
    // Apply a Hann window. The function includes a correction factor
    // to remove the attenuation in the window.
    for (unsigned n = 0; n < N; n++) {
        (in[0])[n] *= hannWindow[n];
    }
    for (unsigned n = 0; n < N; n++) {
        (in[1])[n] *= hannWindow[n];
    }

    kiss_fftr(fftCfg, in[0], transform[0]);
    kiss_fftr(fftCfg, in[1], transform[1]);
    setLed(LED_GREEN, false);
}

static void calculateThd(struct ThdResult* result, unsigned channel)
{
    const float fundBin = signalW[channel] * N;
    const int peakwidth = N/256;
    double fundamental = 0;
    double harms = 0;
    double other = 0;

    for (int n = 1; n < N/2; n++) {
        const float p = normPower(transform[channel][n]);
        if (n >= fundBin - peakwidth && n <= fundBin + peakwidth) {
            fundamental += p;
            goto next;
        }

        for (int h = 2; h < 7; h++) {
            const float harmBin = h * fundBin;
            if (n >= harmBin - peakwidth && n <= harmBin + peakwidth) {
                harms += p;
                goto next;
            }
        }

        // This bin doesn't correspond to a measured peak, and is not DC
        other += p;
        next: ;
    }

    result->dc = normPower(transform[channel][0]);
    result->fundamental = fundamental;
    result->harmonics = harms;
    result->thd = harms / fundamental;
    result->other = other;
}

static void runThdTest(float f)
{
    signalW[0] = f / CODEC_SAMPLERATE;
    signalW[1] = f / CODEC_SAMPLERATE;

    for (unsigned i = 0; i < 20; i++) {
        doFft();
    }

    const unsigned iterations = 20;
    struct ThdResult thd[2] = { };
    for (unsigned i = 0; i < iterations; i++) {
        doFft();
        for (unsigned c = 0; c < 2; c++) {
            struct ThdResult onethd;
            calculateThd(&onethd, c);
            thd[c].dc += onethd.dc;
            thd[c].fundamental += onethd.fundamental;
            thd[c].harmonics += onethd.harmonics;
            thd[c].other += onethd.other;
            thd[c].thd += onethd.thd;
        }
    }
    for (unsigned c = 0; c < 2; c++) {
        thd[c].dc /= iterations;
        thd[c].fundamental /= iterations;
        thd[c].harmonics /= iterations;
        thd[c].other /= iterations;
        thd[c].thd /= iterations;
        printf("%c THD @ %4d Hz: DC:%4d, Other=%d dB, Fundamental=%d dB, harmonics=%d dB, THD=%d dB\n",
                c == 0 ? 'L' : 'R',
                        (int)f, (int)dB(thd[c].dc), (int)dB(thd[c].other),
                        (int)dB(thd[c].fundamental), (int)dB(thd[c].harmonics),
                        (int)dB(thd[c].thd));
    }

    /*
    for (int n = 1; n < N/4; n += 4) {
        const int db = dB(normPower(transform[n]));
        char s[130];

        char* pt = s;
        for (int i = -120; i < db; i++) {
            *pt++ = 'X';
        }
        *pt = '\0';

        printf("%4u %s\n", (n * CODEC_SAMPLERATE) / N, s);
    }
    */
}

static void runCrosstalkTest(float f, unsigned channel)
{
    memset(signalW, 0, sizeof(signalW));
    signalW[channel] = f / CODEC_SAMPLERATE;

    const float fundBin = signalW[channel] * N;
    const int peakwidth = N/256;

    for (unsigned i = 0; i < 20; i++) {
        doFft();
    }

    const unsigned iterations = 20;
    double signal[2] = { 0, 0 };
    for (unsigned i = 0; i < iterations; i++) {
        doFft();
        for (int n = fundBin - peakwidth; n <= fundBin + peakwidth; n++) {
            signal[0] += normPower(transform[0][n]);
            signal[1] += normPower(transform[1][n]);
        };
    }

    signal[0] /= iterations;
    signal[1] /= iterations;

    printf("%c CROSSTALK @ %4d Hz: L %4d dB, R %4d dB, delta %4d dB\n",
            channel == 0 ? 'L' : 'R',
            (int)f,
            (int)dB(signal[0]), (int)dB(signal[1]),
            (int)dB(signal[(channel+1)%2] / signal[channel]));
}

static void runNoisefloorTest()
{
    memset(signalW, 0, sizeof(signalW));

    for (unsigned i = 0; i < 50; i++) {
        doFft();
    }

    double bins[2][10] = { };
    unsigned fHz = 100;
    size_t bin = 0;
    const unsigned iterations = 20;
    for (unsigned i = 0; i < iterations; i++) {
        doFft();
        for (int n = 0; n < N/2; n++) {
            if (n * ((float)CODEC_SAMPLERATE / N) >= fHz) {
                fHz *= 2;
                bin++;
            }
            bins[0][bin] += normPower(transform[0][n]);
            bins[1][bin] += normPower(transform[1][n]);
        }
    }

    printf("  octave up to 100  200  400  800  1k6  3k2  6k4 12k8  24k Hz\n");
    for (unsigned c = 0; c < 2; c++) {
        printf("%c Noisefloor: %4d %4d %4d %4d %4d %4d %4d %4d %4d dB\n",
                c == 0 ? 'L' : 'R',
                        (int)dB(bins[c][0]), (int)dB(bins[c][1]),
                        (int)dB(bins[c][2]), (int)dB(bins[c][3]),
                        (int)dB(bins[c][4]), (int)dB(bins[c][5]),
                        (int)dB(bins[c][6]), (int)dB(bins[c][7]),
                        (int)dB(bins[c][8]));
    }
}

static void runTests()
{
    static const int volumes[] = { -40, -20, -5, 0 };
    static const unsigned fs[] = { 440, 880, 1760, 3520, 7040 };

    while (true) {
        codedSetOutVolume(-70);
        runNoisefloorTest();
        printf("\n");

        printf("Volume %d dB\n", 0);
        codedSetOutVolume(0);
        runCrosstalkTest(600, 0);
        runCrosstalkTest(600, 1);
        runCrosstalkTest(8000, 0);
        runCrosstalkTest(8000, 1);
        runCrosstalkTest(20000, 0);
        runCrosstalkTest(20000, 1);
        printf("\n");

        printf("Volume %d dB\n", -30);
        codedSetOutVolume(-30);
        runCrosstalkTest(600, 0);
        runCrosstalkTest(600, 1);
        runCrosstalkTest(8000, 0);
        runCrosstalkTest(8000, 1);
        runCrosstalkTest(20000, 0);
        runCrosstalkTest(20000, 1);
        printf("\n");

        // Run THD tests
        for (unsigned v = 0; v < sizeof(volumes)/sizeof(*volumes); v++) {
            int vol = volumes[v];
            printf("Volume %d dB\n", vol);
            codedSetOutVolume(vol);

            for (unsigned fi = 0; fi < sizeof(fs)/sizeof(*fs); fi++) {
                unsigned f = fs[fi];
                runThdTest(f);
            }
            printf("\n");
        }
    }
}

int main()
{
    platformInit(NULL);

    printf("Starting test\n");

    fftCfg = kiss_fftr_alloc(N, false, NULL, NULL);

    codedSetOutVolume(0);
    codecRegisterProcessFunction(process);
    platformRegisterIdleCallback(runTests);
    platformMainloop();

    return 0;
}
