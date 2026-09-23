// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 JDT Engineering
// Sixel output of Doom's indexed framebuffer and its original 256-colour palette.
#include "doomgeneric.h"
#include "doomdef.h"
#include "i_video.h"
#include "m_argv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char palette[256][3];
static char output[16384];
static unsigned output_used;

static void FlushOutput(void)
{
    if (output_used) fwrite(output, 1, output_used, stdout);
    output_used = 0;
}

static void Emit(char c)
{
    if (output_used == sizeof output) FlushOutput();
    output[output_used++] = c;
}

// Sixel integers are nonnegative; avoid printf's format parsing in the hot loop.
static void Number(unsigned value)
{
    char digits[10];
    unsigned count = 0;
    do { digits[count++] = '0' + value % 10; value /= 10; } while (value);
    while (count) Emit(digits[--count]);
}

static void Text(const char *text)
{
    while (*text) Emit(*text++);
}

void DG_SetSixelColor(unsigned index, unsigned r, unsigned g, unsigned b)
{
    palette[index][0] = r;
    palette[index][1] = g;
    palette[index][2] = b;
}

void DG_DrawSixel(void)
{
    // Horizontal scaling is applied to runs, so only encode native columns.
    static unsigned char planes[256][SCREENWIDTH];
    static int scale;
    static int first_frame = 1;
    // The game loop already waits for simulation ticks. Present every rendered
    // frame instead of discarding frames with an additional 15 Hz limiter.
    if (!scale) {
        int arg = M_CheckParmWithArgs("-pixel-scale", 1);
        scale = arg ? atoi(myargv[arg + 1]) : 2;
        if (scale < 1 || scale > 3) scale = 2;
    }
    unsigned width = SCREENWIDTH * scale, height = SCREENHEIGHT * scale;
    if (first_frame) {
        Text("\033[2J");
        first_frame = 0;
    }
    // Anchor every frame; declare square pixels, opaque background, exact raster size.
    Text("\033[H\033P0;0;0q\"1;1;"); Number(width); Emit(';'); Number(height);
    for (unsigned c = 0; c < 256; ++c) {
        Emit('#'); Number(c); Text(";2");
        for (unsigned component = 0; component < 3; ++component) {
            Emit(';'); Number((palette[c][component] * 100 + 127) / 255);
        }
    }
    for (unsigned band = 0; band < height; band += 6) {
        unsigned ends[256] = {0};
        memset(planes, 0, sizeof planes);
        for (unsigned bit = 0; bit < 6 && band + bit < height; ++bit) {
            const unsigned char *row = I_VideoBuffer + ((band + bit) / scale) * SCREENWIDTH;
            for (unsigned x = 0; x < SCREENWIDTH; ++x) {
                unsigned c = row[x];
                planes[c][x] |= 1U << bit;
                if (ends[c] < x + 1) ends[c] = x + 1;
            }
        }
        int first = 1;
        for (unsigned c = 0; c < 256; ++c) {
            if (!ends[c]) continue;
            if (!first) Emit('$');
            first = 0;
            Emit('#'); Number(c);
            for (unsigned x = 0; x < ends[c];) {
                unsigned end = x + 1;
                unsigned char value = planes[c][x];
                while (end < ends[c] && planes[c][end] == value) ++end;
                unsigned count = (end - x) * scale;
                if (count >= 4) { Emit('!'); Number(count); Emit(value + 63); }
                else while (count--) Emit(value + 63);
                x = end;
            }
        }
        if (band + 6 < height) Emit('-');
    }
    Text("\033\\");
    FlushOutput();
    fflush(stdout);
}
