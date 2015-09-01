#include <jack/jack.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#include "codec.h"
#include "jackclient.h"

static jack_client_t* client;
static bool die;
static jack_port_t* ol;
static jack_port_t* or;
static jack_port_t* il;
static jack_port_t* ir;
static CodecProcess appProcess;

static int process(jack_nframes_t nframes, void* arg)
{
    (void)arg;

    jack_default_audio_sample_t* ilBuf = jack_port_get_buffer(il, nframes);
    jack_default_audio_sample_t* irBuf = jack_port_get_buffer(ir, nframes);
    jack_default_audio_sample_t* olBuf = jack_port_get_buffer(ol, nframes);
    jack_default_audio_sample_t* orBuf = jack_port_get_buffer(or, nframes);

    // Jack samples are 32-bit float -1..1, we want 16-bit signed integers
    const float scale = 0x8000;
    const float invscale = 1.0/0x8000;

    for (size_t subframe = 0; subframe < nframes; subframe += CODEC_SAMPLES_PER_FRAME) {
        AudioBuffer in;
        AudioBuffer out;

        for (size_t sample = 0; sample < CODEC_SAMPLES_PER_FRAME; sample++) {
            in.s[sample][0] = ilBuf[subframe + sample] * scale;
            in.s[sample][1] = irBuf[subframe + sample] * scale;
        }

        appProcess(&in, &out);

        for (size_t sample = 0; sample < CODEC_SAMPLES_PER_FRAME; sample++) {
            olBuf[subframe + sample] = out.s[sample][0] * invscale;
            orBuf[subframe + sample] = out.s[sample][1] * invscale;
        }
    }

    return 0;
}

static void sigterm(int sig)
{
    (void)sig;
    die = true;
}

void codecRegisterProcessFunction(CodecProcess fn)
{
    appProcess = fn;
}

void jackClientInit()
{
    client = jack_client_open("m4audio", JackNullOption, 0);

    if (!client) {
        fprintf(stderr, "Failed to connect to Jack\n");
        exit(1);
    }

    ol = jack_port_register(client, "left-out",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    or = jack_port_register(client, "right-out",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    il = jack_port_register(client, "left-in",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    ir = jack_port_register(client, "right-in",
            JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    if (!ol || !or || !il || !ir) {
        fprintf(stderr, "Failed to create Jack ports\n");
        exit(1);
    }

    jack_set_process_callback(client, process, NULL);
}

void jackClientRun()
{
    fprintf(stderr, "Running Jack\n");

    if (jack_activate(client)) {
        fprintf(stderr, "Failed to start Jack processing\n");
        exit(1);
    }

    // Attempt to connect the ports to default input and output
    jack_connect(client, "system:capture_1", jack_port_name(il));
    jack_connect(client, "system:capture_2", jack_port_name(ir));
    jack_connect(client, jack_port_name(ol), "system:playback_1");
    jack_connect(client, jack_port_name(or), "system:playback_2");

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    while (!die) {
        sleep(1);
    }
    jack_client_close(client);
}
