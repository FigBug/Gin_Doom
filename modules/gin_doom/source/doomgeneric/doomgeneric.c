#include "doomgeneric.h"
#include "data.h"

uint32_t* DG_ScreenBuffer = 0;

data_t* DG_Alloc()
{
	data_t* data;
	data = malloc (sizeof (data_t));
	memset (data, 0, sizeof (data_t));

	return data;
}

void DG_Free(data_t* data)
{
	free (data);
}

void dg_Create (data_t* data)
{
	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	DG_Init (data);
}

