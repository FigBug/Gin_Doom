#include "doomgeneric.h"
#include "data.h"
#include "m_cheat.h"

data_t* DG_Alloc()
{
	data_t* data;
	data = malloc (sizeof (data_t));
	memset (data, 0, sizeof (data_t));

	// d_main.c
	data->main_loop_started = false;
	data->show_endoom = 1;

	// am_map.c
	cheatseq_t cheat_amap = CHEAT("iddt", 0);

	data->am_map.leveljuststarted = 1; 	// kluge until AM_LevelInit() is called
	data->am_map.finit_width = SCREENWIDTH;
	data->am_map.finit_height = SCREENHEIGHT - 32;
	data->am_map.scale_mtof = (fixed_t)INITSCALEMTOF;
	data->am_map.markpointnum = 0; // next point to be assigned
	data->am_map.followplayer = 1; // specifies whether to follow the player around
	data->am_map.cheat_amap = cheat_amap;
	data->am_map.stopped = true;

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

