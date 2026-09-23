// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the real MUS parser, sample mixer and optional synthesizer.
#include "../src/i_sound_stream.c"
#include <assert.h>
static uint32_t fake_ms;
static unsigned char lump_data[4040];
int myargc;
char **myargv;
uint32_t DG_GetTicksMs(void) { return fake_ms; }
int M_CheckParm(char *name) { return strcmp(name, "-audio") == 0; }
int M_CheckParmWithArgs(char *name, int count) { (void)name; (void)count; return 0; }
int W_CheckNumForName(char *name) { (void)name; return 0; }
int W_LumpLength(unsigned int lump) { (void)lump; return sizeof lump_data; }
void W_ReadLump(unsigned int lump, void *data) { (void)lump; memcpy(data, lump_data, sizeof lump_data); }
int main(int argc, char **argv)
{
    unsigned char mus[] = {'M','U','S',26,12,0,16,0,1,0,0,0,0,0,0,0,
                          0x40,0,30,0x90,0xbc,100,0x81,0x0c,0x80,60,1,0x60};
    struct song *song = RegisterSong(mus, sizeof mus);
    assert(song && song->count == 3 && song->duration == (uint64_t)141 * RATE / 140);
    assert(song->events[0].type == 4 && song->events[0].b == 30);
    assert(song->events[1].type == 1 && song->events[1].a == 60 && song->events[1].b == 100);
    assert(song->events[2].sample == RATE);
    assert(!RegisterSong(mus, sizeof mus - 1));
    mus[16] = 0x50; assert(!RegisterSong(mus, sizeof mus)); mus[16] = 0x40;
    if (argc > 1) {
        synth = tsf_load_filename(argv[1]); assert(synth);
        tsf_set_output(synth, TSF_STEREO_INTERLEAVED, RATE, -10);
        tsf_set_max_voices(synth, 24);
        PlaySong(song, true);
        short samples[BLOCK * 2]; long long energy = 0;
        for (int block = 0; block < 400; ++block) {
            RenderMusic(samples, BLOCK);
            for (int i = 0; i < BLOCK * 2; ++i) energy += (long long)samples[i] * samples[i];
        }
        assert(energy > 0 && playing == song); // crossed the loop boundary
        Pause(); RenderMusic(samples, BLOCK);
        for (int i = 0; i < BLOCK * 2; ++i) assert(samples[i] == 0);
        Resume(); StopMusic(); MusicShutdown();
    }
    UnregisterSong(song);
    memset(lump_data, 255, sizeof lump_data);
    lump_data[0] = 3; lump_data[1] = 0;
    lump_data[2] = 0x11; lump_data[3] = 0x2b; // 11025 Hz
    lump_data[4] = 0xc0; lump_data[5] = 0x0f; lump_data[6] = lump_data[7] = 0; // 4032 samples
    sfxinfo_t effect = {0}; strcpy(effect.name, "test");
    assert(StartEffect(&effect, 0, 127, 0) == 0);
    int pipefd[2]; assert(pipe(pipefd) == 0);
    output_fd = pipefd[1]; audio_ready = 1; fake_ms = 10;
    AudioUpdate();
    unsigned char pcm[4096]; int count = read(pipefd[0], pcm, sizeof pcm);
    assert(count == 220 * 4);
    for (int i = 0; i < count; i += 4) {
        assert(le16(pcm+i) == 32512); // full-volume positive sample, hard left
        assert(le16(pcm+i+2) == 0);
    }
    close(pipefd[0]); close(pipefd[1]); output_fd = -1; audio_ready = 0;
    StopEffect(0); assert(!EffectPlaying(0));
    puts("MUS timing/validation, optional music synthesis/looping, PCM mixing: PASS");
    return 0;
}
