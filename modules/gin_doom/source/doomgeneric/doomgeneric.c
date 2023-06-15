#include "doomgeneric.h"
#include "data.h"

data_t* DG_Alloc()
{
	data_t* data;
	data = malloc (sizeof (data_t));
	memset (data, 0, sizeof (data_t));

	// d_main.c
	data->main_loop_started = false;
	data->show_endoom = 1;

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

