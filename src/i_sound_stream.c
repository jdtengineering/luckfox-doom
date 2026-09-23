// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 JDT Engineering
// Optional stereo PCM output over a FIFO; no sound card or SDL dependency.
#if !defined(_WIN32) && !defined(WIN32)
#include "doomgeneric.h"
#include "i_sound.h"
#include "m_argv.h"
#include "w_wad.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define TSF_IMPLEMENTATION
#include "third_party/tsf.h"

#define RATE 22050
#define CHANNELS 32
#define FIFO_PATH "/tmp/doom-audio.pcm"
#define BLOCK 128
static int output_fd = -1, audio_ready;
static uint32_t last_ms;
static unsigned fractional;
static struct sigaction old_sigpipe;
static snddevice_t devices[] = {SNDDEVICE_SB};

struct effect { unsigned char *data; unsigned length, rate; uint64_t position; int left, right; };
static struct effect effects[CHANNELS];
struct event { uint64_t sample; unsigned char type, channel, a, b; };
struct song { struct event *events; unsigned count; uint64_t duration; };
static tsf *synth;
static struct song *playing;
static unsigned event_index;
static uint64_t music_sample;
static int music_loop, music_paused, music_volume = 100;

static unsigned le16(const unsigned char *p) { return p[0] | p[1] << 8; }
static unsigned le32(const unsigned char *p) { return le16(p) | (uint32_t)le16(p+2) << 16; }
static int enabled(void) { return M_CheckParm("-audio") != 0; }

static bool AudioInit(bool prefix)
{
    (void)prefix;
    if (!enabled()) return false;
    if (audio_ready) return true;
    struct stat st;
    if (mkfifo(FIFO_PATH, 0600) && errno != EEXIST) return false;
    if (lstat(FIFO_PATH, &st) || !S_ISFIFO(st.st_mode) || st.st_uid != getuid()) return false;
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &action, &old_sigpipe);
    last_ms = DG_GetTicksMs();
    audio_ready = 1;
    return true;
}

static void StopEffect(int channel)
{
    if (channel < 0 || channel >= CHANNELS) return;
    free(effects[channel].data);
    memset(&effects[channel], 0, sizeof effects[channel]);
}
static void AudioShutdown(void)
{
    if (!audio_ready) return;
    for (int i = 0; i < CHANNELS; ++i) StopEffect(i);
    if (output_fd >= 0) close(output_fd);
    output_fd = -1;
    unlink(FIFO_PATH);
    sigaction(SIGPIPE, &old_sigpipe, NULL);
    audio_ready = 0;
}
static int Lump(sfxinfo_t *sfx)
{
    char name[12];
    if (sfx->link) sfx = sfx->link;
    snprintf(name, sizeof name, "ds%.8s", sfx->name);
    return W_CheckNumForName(name);
}
static void Params(int channel, int volume, int separation)
{
    if (channel < 0 || channel >= CHANNELS) return;
    effects[channel].left = volume * (254 - separation) / 254;
    effects[channel].right = volume * separation / 254;
}
static int StartEffect(sfxinfo_t *sfx, int channel, int volume, int separation)
{
    if (channel < 0 || channel >= CHANNELS) return -1;
    StopEffect(channel);
    int lump = Lump(sfx);
    if (lump < 0) return -1;
    int length = W_LumpLength(lump);
    if (length < 40 || length > 1024 * 1024) return -1;
    unsigned char *data = malloc(length);
    if (!data) return -1;
    W_ReadLump(lump, data);
    unsigned samples = le32(data + 4), rate = le16(data + 2);
    if (le16(data) != 3 || !rate || samples < 32 || samples > (unsigned)length - 8) {
        free(data); return -1;
    }
    // DMX samples have 16 bytes of leading and trailing padding.
    memmove(data, data + 24, samples - 32);
    effects[channel].data = data;
    effects[channel].length = samples - 32;
    effects[channel].rate = rate;
    Params(channel, volume, separation);
    return channel;
}
static bool EffectPlaying(int channel)
{
    return channel >= 0 && channel < CHANNELS && effects[channel].data != NULL;
}

static void ResetMusic(void)
{
    tsf_reset(synth);
    for (int ch = 0; ch < 16; ++ch) {
        tsf_channel_set_presetnumber(synth, ch, 0, ch == 9);
        tsf_channel_set_volume(synth, ch, 1.0f);
        tsf_channel_set_pan(synth, ch, 0.5f);
        tsf_channel_set_pitchwheel(synth, ch, 8192);
    }
    event_index = 0;
    music_sample = 0;
}
static bool MusicInit(void)
{
    if (!enabled() || !AudioInit(true)) return false;
    int arg = M_CheckParmWithArgs("-soundfont", 1);
    const char *path = arg ? myargv[arg + 1] : "TimGM6mb.sf2";
    synth = tsf_load_filename(path);
    if (!synth) { fprintf(stderr, "Music: cannot load %s; effects remain available.\n", path); return false; }
    tsf_set_output(synth, TSF_STEREO_INTERLEAVED, RATE, -10.0f);
    tsf_set_max_voices(synth, 24);
    ResetMusic();
    return true;
}
static void MusicShutdown(void) { if (synth) tsf_close(synth); synth = NULL; playing = NULL; }
static void Volume(int volume) { music_volume = volume; }
static void Pause(void) { music_paused = 1; }
static void Resume(void) { music_paused = 0; }
static void *RegisterSong(void *raw, int length)
{
    const unsigned char *data = raw;
    if (length < 16 || memcmp(data, "MUS\x1a", 4)) return NULL;
    unsigned start = le16(data + 6), size = le16(data + 4);
    if (start < 16 || start > (unsigned)length || size > (unsigned)length - start) return NULL;
    struct song *song = calloc(1, sizeof *song);
    if (!song) return NULL;
    song->events = calloc(size + 1, sizeof *song->events);
    if (!song->events) { free(song); return NULL; }
    unsigned pos = start, end = start + size;
    unsigned char velocities[16]; memset(velocities, 127, sizeof velocities);
    uint64_t tick = 0;
    int ended = 0;
    while (pos < end) {
        unsigned descriptor = data[pos++], type = (descriptor >> 4) & 7;
        unsigned channel = descriptor & 15;
        struct event e = {tick * RATE / 140, type, channel == 15 ? 9 : channel == 9 ? 15 : channel, 0, 0};
        if (type == 6) { ended = 1; break; }
        if (type == 5 || type == 7 || pos >= end) goto invalid;
        e.a = data[pos++];
        if (type == 1) {
            if (e.a & 128) {
                if (pos >= end) goto invalid;
                velocities[channel] = data[pos++] & 127;
            }
            e.a &= 127; e.b = velocities[channel];
        } else if (type == 4) {
            if (pos >= end || e.a > 9) goto invalid;
            e.b = data[pos++] & 127;
        } else if (type == 3 && (e.a < 10 || e.a > 14)) goto invalid;
        else if (type == 0) e.a &= 127;
        song->events[song->count++] = e;
        if (descriptor & 128) {
            unsigned delay = 0, count = 0, value;
            do {
                if (pos >= end || ++count > 4) goto invalid;
                value = data[pos++]; delay = (delay << 7) | (value & 127);
            } while (value & 128);
            tick += delay;
        }
    }
    if (!ended || !tick) goto invalid;
    song->duration = tick * RATE / 140;
    return song;
invalid:
    free(song->events); free(song); return NULL;
}
static void StopMusic(void) { playing = NULL; if (synth) tsf_reset(synth); }
static void UnregisterSong(void *handle)
{
    struct song *song = handle;
    if (!song) return;
    if (playing == song) StopMusic();
    free(song->events); free(song);
}
static void PlaySong(void *handle, bool loop)
{
    playing = handle; music_loop = loop; music_paused = 0;
    if (synth) ResetMusic();
}
static bool MusicPlaying(void) { return playing != NULL; }
static void ApplyEvent(const struct event *e)
{
    static const unsigned char controllers[] = {0, 0, 1, 7, 10, 11, 91, 93, 64, 67, 120, 123, 126, 127, 121};
    switch (e->type) {
    case 0: tsf_channel_note_off(synth, e->channel, e->a); break;
    case 1: tsf_channel_note_on(synth, e->channel, e->a, e->b / 127.0f); break;
    case 2: tsf_channel_set_pitchwheel(synth, e->channel, e->a * 64); break;
    case 3: tsf_channel_midi_control(synth, e->channel, controllers[e->a], 0); break;
    case 4:
        if (!e->a) tsf_channel_set_presetnumber(synth, e->channel, e->b, e->channel == 9);
        else tsf_channel_midi_control(synth, e->channel, controllers[e->a], e->b);
        break;
    }
}
static void RenderMusic(short *output, unsigned count)
{
    memset(output, 0, count * 2 * sizeof(short));
    if (!synth || !playing || music_paused) return;
    unsigned offset = 0;
    while (offset < count && playing) {
        if (music_sample >= playing->duration) {
            if (music_loop) ResetMusic(); else { StopMusic(); break; }
        }
        while (event_index < playing->count && playing->events[event_index].sample <= music_sample)
            ApplyEvent(&playing->events[event_index++]);
        uint64_t boundary = event_index < playing->count ? playing->events[event_index].sample : playing->duration;
        unsigned segment = count - offset;
        if (boundary - music_sample < segment) segment = boundary - music_sample;
        if (!segment) break;
        tsf_render_short(synth, output + offset * 2, segment, 0);
        offset += segment; music_sample += segment;
    }
    for (unsigned i = 0; i < count * 2; ++i) output[i] = output[i] * music_volume / 127;
}
static int Clip(int value) { return value < -32768 ? -32768 : value > 32767 ? 32767 : value; }
static void AudioUpdate(void)
{
    if (!audio_ready) return;
    uint32_t now = DG_GetTicksMs(), delta = now - last_ms;
    last_ms = now;
    if (delta > 200) delta = 200;
    unsigned samples = delta * RATE + fractional;
    fractional = samples % 1000; samples /= 1000;
    if (output_fd < 0) {
        output_fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
#ifdef F_SETPIPE_SZ
        if (output_fd >= 0) fcntl(output_fd, F_SETPIPE_SZ, 16384);
#endif
    }
    while (samples) {
        unsigned count = samples < BLOCK ? samples : BLOCK;
        short music[BLOCK * 2]; unsigned char pcm[BLOCK * 4];
        RenderMusic(music, count);
        for (unsigned i = 0; i < count; ++i) {
            int left = music[i*2], right = music[i*2+1];
            for (int ch = 0; ch < CHANNELS; ++ch) {
                struct effect *e = &effects[ch];
                if (!e->data) continue;
                unsigned index = e->position / RATE;
                if (index >= e->length) { StopEffect(ch); continue; }
                int sample = ((int)e->data[index] - 128) * 256;
                left += sample * e->left / 127;
                right += sample * e->right / 127;
                e->position += e->rate;
            }
            uint16_t l = (uint16_t)Clip(left), r = (uint16_t)Clip(right);
            pcm[i*4] = l; pcm[i*4+1] = l >> 8; pcm[i*4+2] = r; pcm[i*4+3] = r >> 8;
        }
        if (output_fd >= 0 && write(output_fd, pcm, count * 4) < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            close(output_fd); output_fd = -1;
        }
        samples -= count;
    }
}

sound_module_t sound_stream_module = {devices, 1, AudioInit, AudioShutdown, Lump,
    AudioUpdate, Params, StartEffect, StopEffect, EffectPlaying, NULL};
music_module_t music_stream_module = {devices, 1, MusicInit, MusicShutdown, Volume,
    Pause, Resume, RegisterSong, UnregisterSong, PlaySong, StopMusic, MusicPlaying, AudioUpdate};
#endif
