#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdlib.h>
#include <stdint.h>

#include "data.h"

#define DOOMGENERIC_RESX 640
#define DOOMGENERIC_RESY 400


data_t* DG_Alloc();
void DG_Free(data_t* data);

void DG_Init (data_t* data);
void DG_DrawFrame (data_t* data);
void DG_SleepMs (data_t* data, uint32_t ms);
uint32_t DG_GetTicksMs (data_t* data);
int DG_GetKey(data_t* data, int* pressed, unsigned char* key);
void DG_SetWindowTitle(data_t* data, const char * title);

#endif //DOOM_GENERIC
