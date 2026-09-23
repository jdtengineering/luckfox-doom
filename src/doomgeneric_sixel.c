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

void DG_SetSixelColor(unsigned index, unsigned r, unsigned g, unsigned b)
{
    palette[index][0] = r;
    palette[index][1] = g;
    palette[index][2] = b;
}

void DG_DrawSixel(void)
{
    static unsigned char planes[256][SCREENWIDTH * 3];
    static uint32_t previous_ms;
    static int scale;
    uint32_t ms = DG_GetTicksMs();
    // Limit terminal traffic while leaving game simulation and input responsive.
    if (previous_ms && (uint32_t)(ms - previous_ms) < 66) return;
    previous_ms = ms;
    if (!scale) {
        int arg = M_CheckParmWithArgs("-pixel-scale", 1);
        scale = arg ? atoi(myargv[arg + 1]) : 2;
        if (scale < 1 || scale > 3) scale = 2;
    }
    unsigned width = SCREENWIDTH * scale, height = SCREENHEIGHT * scale;
    // Anchor every frame; declare square pixels, opaque background, exact raster size.
    printf("\033[H\033P0;0;0q\"1;1;%u;%u", width, height);
    for (unsigned c = 0; c < 256; ++c)
        printf("#%u;2;%u;%u;%u", c, (palette[c][0] * 100 + 127) / 255,
               (palette[c][1] * 100 + 127) / 255, (palette[c][2] * 100 + 127) / 255);
    for (unsigned band = 0; band < height; band += 6) {
        unsigned ends[256] = {0};
        memset(planes, 0, sizeof planes);
        for (unsigned bit = 0; bit < 6 && band + bit < height; ++bit) {
            const unsigned char *row = I_VideoBuffer + ((band + bit) / scale) * SCREENWIDTH;
            for (unsigned x = 0; x < width; ++x) {
                unsigned c = row[x / scale];
                planes[c][x] |= 1U << bit;
                if (ends[c] < x + 1) ends[c] = x + 1;
            }
        }
        int first = 1;
        for (unsigned c = 0; c < 256; ++c) {
            if (!ends[c]) continue;
            if (!first) putchar('$');
            first = 0;
            printf("#%u", c);
            for (unsigned x = 0; x < ends[c];) {
                unsigned end = x + 1;
                unsigned char value = planes[c][x];
                while (end < ends[c] && planes[c][end] == value) ++end;
                unsigned count = end - x;
                if (count >= 4) printf("!%u%c", count, value + 63);
                else while (count--) putchar(value + 63);
                x = end;
            }
        }
        if (band + 6 < height) putchar('-');
    }
    fputs("\033\\", stdout);
    fflush(stdout);
}
