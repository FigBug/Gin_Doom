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
//	Archiving: SaveGame I/O.
//	Thinker, Ticker.
//


#include "z_zone.h"
#include "p_local.h"

#include "doomstat.h"



//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//



// Both the head and tail of the thinker list.


//
// P_InitThinkers
//
void P_InitThinkers (data_t* data)
{
    data->thinkercap.prev = data->thinkercap.next  = &data->thinkercap;
}




//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker (data_t* data, thinker_t* thinker)
{
    data->thinkercap.prev->next = thinker;
    thinker->next = &data->thinkercap;
    thinker->prev = data->thinkercap.prev;
    data->thinkercap.prev = thinker;
}



//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
void P_RemoveThinker (data_t* data, thinker_t* thinker)
{
  // FIXME: NOP.
  thinker->function.acv = (actionf_v)(-1);
}



//
// P_AllocateThinker
// Allocates memory and adds a new thinker at the end of the list.
//
void P_AllocateThinker (data_t* data, thinker_t* thinker)
{
}



//
// P_RunThinkers
//
void P_RunThinkers (data_t* data)
{
    thinker_t*	currentthinker;

    currentthinker = data->thinkercap.next;
    while (currentthinker != &data->thinkercap)
    {
	if ( currentthinker->function.acv == (actionf_v)(-1) )
	{
	    // time to remove it
	    currentthinker->next->prev = currentthinker->prev;
	    currentthinker->prev->next = currentthinker->next;
	    Z_Free (currentthinker);
	}
	else
	{
	    if (currentthinker->function.acp1)
		currentthinker->function.acp1 (data, currentthinker);
	}
	currentthinker = currentthinker->next;
    }
}



//
// P_Ticker
//

void P_Ticker (data_t* data)
{
    int		i;
    
    // run the tic
    if (data->paused)
	return;
		
    // pause if in menu and at least one tic has been run
    if ( !data->netgame
	 && data->menuactive
	 && !data->demoplayback
	 && data->players[data->consoleplayer].viewz != 1)
    {
	return;
    }
    
		
    for (i=0 ; i<MAXPLAYERS ; i++)
	if (data->playeringame[i])
	    P_PlayerThink (data, &data->players[i]);
			
    P_RunThinkers (data);
    P_UpdateSpecials (data);
    P_RespawnSpecials (data);

    // for par times
    data->leveltime++;	
}
