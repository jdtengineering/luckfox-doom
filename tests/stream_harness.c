// SPDX-License-Identifier: GPL-2.0-or-later
#include "doomgeneric.h"
unsigned char frame[320 * 200];
unsigned char *I_VideoBuffer = frame;
uint32_t DG_GetTicksMs(void) { return 1234; }
int M_CheckParm(char *name) { (void)name; return 1; }
int main(void)
{
    DG_InitStream();
    for (unsigned c = 0; c < 256; ++c) DG_SetStreamColor(c, c, 255-c, c/2);
    for (unsigned i = 0; i < sizeof frame; ++i) frame[i] = i & 255;
    DG_DrawStream();
    return 0;
}
