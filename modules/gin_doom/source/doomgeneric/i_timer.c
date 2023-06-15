//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Timer functions.
//

#include "i_timer.h"
#include "data.h"
#include "doomtype.h"

#include "doomgeneric.h"

#include <stdarg.h>

//#include <sys/time.h>
//#include <unistd.h>


//
// I_GetTime
// returns time in 1/35th second tics
//

static uint32_t basetime = 0;


int I_GetTicks (data_t* data)
{
	return DG_GetTicksMs (data);
}

int  I_GetTime (data_t* data)
{
    uint32_t ticks;

    ticks = I_GetTicks (data);

    if (basetime == 0)
        basetime = ticks;

    ticks -= basetime;

    return (ticks * TICRATE) / 1000;
}


//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS (data_t* data)
{
    uint32_t ticks;

    ticks = I_GetTicks (data);

    if (basetime == 0)
        basetime = ticks;

    return ticks - basetime;
}

// Sleep for a specified number of ms

void I_Sleep (data_t* data, int ms)
{
    //SDL_Delay(ms);
    //usleep (ms * 1000);

	DG_SleepMs(data, ms);
}

void I_WaitVBL(int count)
{
    //I_Sleep((count * 1000) / 70);
}


void I_InitTimer(void)
{
    // initialize timer

    //SDL_Init(SDL_INIT_TIMER);
}

