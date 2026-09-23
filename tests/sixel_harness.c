// SPDX-License-Identifier: GPL-2.0-or-later
#include "doomgeneric.h"
#include <stdlib.h>
#include <string.h>
unsigned char frame[320 * 200];
unsigned char *I_VideoBuffer = frame;
int myargc;
char **myargv;
uint32_t DG_GetTicksMs(void) { return 1000; }
int M_CheckParmWithArgs(char *name, int count)
{
    (void)name; (void)count;
    return myargc > 2 ? 1 : 0;
}
int main(int argc, char **argv)
{
    myargc = argc; myargv = argv;
    unsigned pattern = argc > 3 ? atoi(argv[3]) : 0;
    for (unsigned y = 0; y < 200; ++y)
        for (unsigned x = 0; x < 320; ++x)
            frame[y * 320 + x] = pattern == 1 ? 37 : pattern == 2
                ? ((x / 7 + y / 3) & 255) : (x * 17 + y * 31) & 255;
    for (unsigned c = 0; c < 256; ++c) DG_SetSixelColor(c, c, 255-c, c/2);
    DG_DrawSixel();
    return 0;
}
