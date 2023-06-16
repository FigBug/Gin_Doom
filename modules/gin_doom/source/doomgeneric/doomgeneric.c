#include "doomgeneric.h"
#include "data.h"
#include "m_cheat.h"

data_t* DG_Alloc()
{
	data_t* data;
	data = malloc (sizeof (data_t));
	memset (data, 0, sizeof (data_t));

	D_Main_Init (data);
	AM_Map_Init (data);
	D_Items_Init (data);
	D_Loop_Init (data);

	return data;
}

void DG_Free(data_t* data)
{
	free (data->DG_ScreenBuffer);

	free (data);
}

void dg_Create (data_t* data)
{
	data->DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	DG_Init (data);
}

