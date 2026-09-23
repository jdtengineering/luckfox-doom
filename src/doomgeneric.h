#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdint.h>

extern unsigned DOOMGENERIC_RESX;
extern unsigned DOOMGENERIC_RESY;

extern uint32_t *DG_ScreenBuffer;

void DG_Init(void);
void DG_DrawFrame(void);
void DG_DrawSixel(void);
extern int DG_StreamActive;
int DG_InitStream(void);
void DG_DrawStream(void);
int DG_StreamKey(int *pressed, unsigned char *key);
void DG_SetStreamColor(unsigned index, unsigned r, unsigned g, unsigned b);
void DG_SetSixelColor(unsigned index, unsigned r, unsigned g, unsigned b);
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs(void);
int DG_GetKey(int *pressed, unsigned char *key);
void DG_SetWindowTitle(const char *title);
void DG_ReadInput(void);

#endif //DOOM_GENERIC
