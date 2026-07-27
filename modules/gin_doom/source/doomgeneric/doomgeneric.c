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
	data->precache = true;
	data->wipegamestate = GS_DEMOSCREEN;
	data->sfxVolume = 8;
	data->validcount = 1;
	data->mouseSensitivity = 5;
	data->gamemission = doom;
	data->gameversion = exe_final2;
	data->snd_channels = 8;
	data->dd_oldgamestate = -1;
	data->sb_pain_oldhealth = -1;
	data->am_lastlevel = -1;
	data->am_lastepisode = -1;
	data->ps_ovr_first = 1;
	data->musicVolume = 8;
	data->mousebuttons = &data->mousearray[1];
	data->joybuttons = &data->joyarray[1];

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

