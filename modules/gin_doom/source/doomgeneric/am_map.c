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
//
// DESCRIPTION:  the automap code
//


#include <stdio.h>

#include "deh_main.h"

#include "z_zone.h"
#include "doomkeys.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "m_cheat.h"
#include "m_controls.h"
#include "m_misc.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"

#include "am_map.h"


// For use if I do walls with outsides/insides
#define REDS		(256-5*16)
#define REDRANGE	16
#define BLUES		(256-4*16+8)
#define BLUERANGE	8
#define GREENS		(7*16)
#define GREENRANGE	16
#define GRAYS		(6*16)
#define GRAYSRANGE	16
#define BROWNS		(4*16)
#define BROWNRANGE	16
#define YELLOWS		(256-32+7)
#define YELLOWRANGE	1
#define BLACK		0
#define WHITE		(256-47)

// Automap colors
#define BACKGROUND	BLACK
#define YOURCOLORS	WHITE
#define YOURRANGE	0
#define WALLCOLORS	REDS
#define WALLRANGE	REDRANGE
#define TSWALLCOLORS	GRAYS
#define TSWALLRANGE	GRAYSRANGE
#define FDWALLCOLORS	BROWNS
#define FDWALLRANGE	BROWNRANGE
#define CDWALLCOLORS	YELLOWS
#define CDWALLRANGE	YELLOWRANGE
#define THINGCOLORS	GREENS
#define THINGRANGE	GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS	(GRAYS + GRAYSRANGE/2)
#define GRIDRANGE	0
#define XHAIRCOLORS	GRAYS

// drawing stuff

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),data->am_map.scale_ftom)
#define MTOF(x) (FixedMul((x),data->am_map.scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (data->am_map.f_x + MTOF((x)-data->am_map.m_x))
#define CYMTOF(y)  (data->am_map.f_y + (data->am_map.f_h - MTOF((y)-data->am_map.m_y)))

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

typedef struct
{
    int x, y;
} fpoint_t;

typedef struct
{
    fpoint_t a, b;
} fline_t;

typedef struct
{
    mpoint_t a, b;
} mline_t;

typedef struct
{
    fixed_t slp, islp;
} islope_t;



//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/4 } },  // ----->
    { { R, 0 }, { R-R/2, -R/4 } },
    { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
    { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/6 } },  // ----->
    { { R, 0 }, { R-R/2, -R/6 } },
    { { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
    { { -R+R/8, 0 }, { -R-R/8, -R/6 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
    { { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
    { { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
    { { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
    { { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
    { { -R/6, -R/6 }, { 0, -R/6 } },
    { { 0, -R/6 }, { 0, R/4 } },
    { { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
    { { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
    { { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R

#define R (FRACUNIT)
mline_t triangle_guy[] = {
    { { (fixed_t)(-.867*R), (fixed_t)(-.5*R) }, { (fixed_t)(.867*R ), (fixed_t)(-.5*R) } },
    { { (fixed_t)(.867*R ), (fixed_t)(-.5*R) }, { (fixed_t)(0      ), (fixed_t)(R    ) } },
    { { (fixed_t)(0      ), (fixed_t)(R    ) }, { (fixed_t)(-.867*R), (fixed_t)(-.5*R) } }
};
#undef R

#define R (FRACUNIT)
mline_t thintriangle_guy[] = {
    { { (fixed_t)(-.5*R), (fixed_t)(-.7*R) }, { (fixed_t)(R    ), (fixed_t)(0    ) } },
    { { (fixed_t)(R    ), (fixed_t)(0    ) }, { (fixed_t)(-.5*R), (fixed_t)(.7*R ) } },
    { { (fixed_t)(-.5*R), (fixed_t)(.7*R ) }, { (fixed_t)(-.5*R), (fixed_t)(-.7*R) } }
};
#undef R

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.

void AM_Map_Init (data_t* data)
{
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
}

void
AM_getIslope
( mline_t*	ml,
  islope_t*	is )
{
    int dx, dy;

    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (!dy) is->islp = (dx<0?-INT_MAX:INT_MAX);
    else is->islp = FixedDiv(dx, dy);
    if (!dx) is->slp = (dy<0?-INT_MAX:INT_MAX);
    else is->slp = FixedDiv(dy, dx);

}

//
//
//
void AM_activateNewScale(data_t* data)
{
    data->am_map.m_x += data->am_map.m_w/2;
    data->am_map.m_y += data->am_map.m_h/2;
    data->am_map.m_w = FTOM(data->am_map.f_w);
    data->am_map.m_h = FTOM(data->am_map.f_h);
	data->am_map.m_x -= data->am_map.m_w/2;
	data->am_map.m_y -= data->am_map.m_h/2;
	data->am_map.m_x2 = data->am_map.m_x + data->am_map.m_w;
	data->am_map.m_y2 = data->am_map.m_y + data->am_map.m_h;
}

//
//
//
void AM_saveScaleAndLoc(data_t* data)
{
	data->am_map.old_m_x = data->am_map.m_x;
	data->am_map.old_m_y = data->am_map.m_y;
	data->am_map.old_m_w = data->am_map.m_w;
	data->am_map.old_m_h = data->am_map.m_h;
}

//
//
//
void AM_restoreScaleAndLoc(data_t* data)
{

	data->am_map.m_w = data->am_map.old_m_w;
	data->am_map.m_h = data->am_map.old_m_h;
    if (!data->am_map.followplayer)
    {
		data->am_map.m_x = data->am_map.old_m_x;
		data->am_map.m_y = data->am_map.old_m_y;
    } else {
		data->am_map.m_x = data->am_map.plr->mo->x - data->am_map.m_w/2;
		data->am_map.m_y = data->am_map.plr->mo->y - data->am_map.m_h/2;
    }
	data->am_map.m_x2 = data->am_map.m_x + data->am_map.m_w;
	data->am_map.m_y2 = data->am_map.m_y + data->am_map.m_h;

    // Change the scaling multipliers
	data->am_map.scale_mtof = FixedDiv(data->am_map.f_w<<FRACBITS, data->am_map.m_w);
	data->am_map.scale_ftom = FixedDiv(FRACUNIT, data->am_map.scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark(data_t* data)
{
	data->am_map.markpoints[data->am_map.markpointnum].x = data->am_map.m_x + data->am_map.m_w/2;
	data->am_map.markpoints[data->am_map.markpointnum].y = data->am_map.m_y + data->am_map.m_h/2;
	data->am_map.markpointnum = (data->am_map.markpointnum + 1) % AM_NUMMARKPOINTS;

}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(data_t* data)
{
    int i;
    fixed_t a;
    fixed_t b;

	data->am_map.min_x = data->am_map.min_y =  INT_MAX;
	data->am_map.max_x = data->am_map.max_y = -INT_MAX;
  
    for (i=0;i<numvertexes;i++)
    {
	if (vertexes[i].x < data->am_map.min_x)
		data->am_map.min_x = vertexes[i].x;
	else if (vertexes[i].x > data->am_map.max_x)
		data->am_map.max_x = vertexes[i].x;
    
	if (vertexes[i].y < data->am_map.min_y)
		data->am_map.min_y = vertexes[i].y;
	else if (vertexes[i].y > data->am_map.max_y)
		data->am_map.max_y = vertexes[i].y;
    }
  
	data->am_map.max_w = data->am_map.max_x - data->am_map.min_x;
	data->am_map.max_h = data->am_map.max_y - data->am_map.min_y;

	data->am_map.min_w = 2*PLAYERRADIUS; // const? never changed?
	data->am_map.min_h = 2*PLAYERRADIUS;

    a = FixedDiv(data->am_map.f_w<<FRACBITS, data->am_map.max_w);
    b = FixedDiv(data->am_map.f_h<<FRACBITS, data->am_map.max_h);
  
	data->am_map.min_scale_mtof = a < b ? a : b;
	data->am_map.max_scale_mtof = FixedDiv(data->am_map.f_h<<FRACBITS, 2*PLAYERRADIUS);

}


//
//
//
void AM_changeWindowLoc(data_t* data)
{
    if (data->am_map.m_paninc.x || data->am_map.m_paninc.y)
    {
		data->am_map.followplayer = 0;
		data->am_map.f_oldloc.x = INT_MAX;
    }

	data->am_map.m_x += data->am_map.m_paninc.x;
	data->am_map.m_y += data->am_map.m_paninc.y;

    if (data->am_map.m_x + data->am_map.m_w/2 > data->am_map.max_x)
		data->am_map.m_x = data->am_map.max_x - data->am_map.m_w/2;
    else if (data->am_map.m_x + data->am_map.m_w/2 < data->am_map.min_x)
		data->am_map.m_x = data->am_map.min_x - data->am_map.m_w/2;
  
    if (data->am_map.m_y + data->am_map.m_h/2 > data->am_map.max_y)
		data->am_map.m_y = data->am_map.max_y - data->am_map.m_h/2;
    else if (data->am_map.m_y + data->am_map.m_h/2 < data->am_map.min_y)
		data->am_map.m_y = data->am_map.min_y - data->am_map.m_h/2;

	data->am_map.m_x2 = data->am_map.m_x + data->am_map.m_w;
	data->am_map.m_y2 = data->am_map.m_y + data->am_map.m_h;
}


//
//
//
void AM_initVariables(data_t* data)
{
    int pnum;
    static event_t st_notify = { ev_keyup, AM_MSGENTERED, 0, 0 };

	data->am_map.automapactive = true;
	data->am_map.fb = I_VideoBuffer;

	data->am_map.f_oldloc.x = INT_MAX;
	data->am_map.amclock = 0;
	data->am_map.lightlev = 0;

	data->am_map.m_paninc.x = data->am_map.m_paninc.y = 0;
	data->am_map.ftom_zoommul = FRACUNIT;
	data->am_map.mtof_zoommul = FRACUNIT;

	data->am_map.m_w = FTOM(data->am_map.f_w);
	data->am_map.m_h = FTOM(data->am_map.f_h);

    // find player to center on initially
    if (playeringame[consoleplayer])
    {
		data->am_map.plr = &players[consoleplayer];
    }
    else
    {
		data->am_map.plr = &players[0];

	for (pnum=0;pnum<MAXPLAYERS;pnum++)
        {
	    if (playeringame[pnum])
            {
				data->am_map.plr = &players[pnum];
		break;
            }
        }
    }

	data->am_map.m_x = data->am_map.plr->mo->x - data->am_map.m_w/2;
	data->am_map.m_y = data->am_map.plr->mo->y - data->am_map.m_h/2;
    AM_changeWindowLoc(data);

    // for saving & restoring
	data->am_map.old_m_x = data->am_map.m_x;
	data->am_map.old_m_y = data->am_map.m_y;
	data->am_map.old_m_w = data->am_map.m_w;
	data->am_map.old_m_h = data->am_map.m_h;

    // inform the status bar of the change
    ST_Responder(&st_notify);

}

//
// 
//
void AM_loadPics(data_t* data)
{
    int i;
    char namebuf[9];
  
    for (i=0;i<10;i++)
    {
		DEH_snprintf(namebuf, 9, "AMMNUM%d", i);
		data->am_map.marknums[i] = W_CacheLumpName(namebuf, PU_STATIC);
    }

}

void AM_unloadPics(data_t* data)
{
    int i;
    char namebuf[9];
  
    for (i=0;i<10;i++)
    {
		DEH_snprintf(namebuf, 9, "AMMNUM%d", i);
		W_ReleaseLumpName(namebuf);
    }
}

void AM_clearMarks(data_t* data)
{
    int i;

    for (i=0;i<AM_NUMMARKPOINTS;i++)
		data->am_map.markpoints[i].x = -1; // means empty
	data->am_map.markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(data_t* data)
{
	data->am_map.leveljuststarted = 0;

	data->am_map.f_x = data->am_map.f_y = 0;
	data->am_map.f_w = data->am_map.finit_width;
	data->am_map.f_h = data->am_map.finit_height;

    AM_clearMarks(data);

    AM_findMinMaxBoundaries(data);
	data->am_map.scale_mtof = FixedDiv(data->am_map.min_scale_mtof, (int) (0.7*FRACUNIT));
    if (data->am_map.scale_mtof > data->am_map.max_scale_mtof)
		data->am_map.scale_mtof = data->am_map.min_scale_mtof;
	data->am_map.scale_ftom = FixedDiv(FRACUNIT, data->am_map.scale_mtof);
}




//
//
//
void AM_Stop (data_t* data)
{
    static event_t st_notify = { 0, ev_keyup, AM_MSGEXITED, 0 };

    AM_unloadPics(data);
	data->am_map.automapactive = false;
    ST_Responder(&st_notify);
	data->am_map.stopped = true;
}

//
//
//
void AM_Start (data_t* data)
{
    static int lastlevel = -1, lastepisode = -1;

    if (!data->am_map.stopped)
		AM_Stop(data);
	data->am_map.stopped = false;

    if (lastlevel != gamemap || lastepisode != gameepisode)
    {
		AM_LevelInit(data);
		lastlevel = gamemap;
		lastepisode = gameepisode;
    }
    AM_initVariables(data);
    AM_loadPics(data);
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale(data_t* data)
{
	data->am_map.scale_mtof = data->am_map.min_scale_mtof;
	data->am_map.scale_ftom = FixedDiv(FRACUNIT, data->am_map.scale_mtof);
    AM_activateNewScale(data);
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale(data_t* data)
{
	data->am_map.scale_mtof = data->am_map.max_scale_mtof;
	data->am_map.scale_ftom = FixedDiv(FRACUNIT, data->am_map.scale_mtof);
    AM_activateNewScale(data);
}


//
// Handle events (user inputs) in automap mode
//
boolean
AM_Responder
( data_t* data, event_t*	ev )
{

    int rc;
    static int bigstate=0;
    static char buffer[20];
    int key;

    rc = false;

    if (!data->am_map.automapactive)
    {
	if (ev->type == ev_keydown && ev->data1 == key_map_toggle)
	{
	    AM_Start (data);
	    viewactive = false;
	    rc = true;
	}
    }
    else if (ev->type == ev_keydown)
    {
	rc = true;
        key = ev->data1;

        if (key == key_map_east)          // pan right
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.x = FTOM(F_PANINC);
            else rc = false;
        }
        else if (key == key_map_west)     // pan left
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.x = -FTOM(F_PANINC);
            else rc = false;
        }
        else if (key == key_map_north)    // pan up
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.y = FTOM(F_PANINC);
            else rc = false;
        }
        else if (key == key_map_south)    // pan down
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.y = -FTOM(F_PANINC);
            else rc = false;
        }
        else if (key == key_map_zoomout)  // zoom out
        {
			data->am_map.mtof_zoommul = M_ZOOMOUT;
			data->am_map.ftom_zoommul = M_ZOOMIN;
        }
        else if (key == key_map_zoomin)   // zoom in
        {
			data->am_map.mtof_zoommul = M_ZOOMIN;
			data->am_map.ftom_zoommul = M_ZOOMOUT;
        }
        else if (key == key_map_toggle)
        {
            bigstate = 0;
            viewactive = true;
            AM_Stop (data);
        }
        else if (key == key_map_maxzoom)
        {
            bigstate = !bigstate;
            if (bigstate)
            {
                AM_saveScaleAndLoc(data);
                AM_minOutWindowScale(data);
            }
            else AM_restoreScaleAndLoc(data);
        }
        else if (key == key_map_follow)
        {
			data->am_map.followplayer = !data->am_map.followplayer;
			data->am_map.f_oldloc.x = INT_MAX;
            if (data->am_map.followplayer)
				data->am_map.plr->message = DEH_String(AMSTR_FOLLOWON);
            else
				data->am_map.plr->message = DEH_String(AMSTR_FOLLOWOFF);
        }
        else if (key == key_map_grid)
        {
			data->am_map.grid = !data->am_map.grid;
            if (data->am_map.grid)
				data->am_map.plr->message = DEH_String(AMSTR_GRIDON);
            else
				data->am_map.plr->message = DEH_String(AMSTR_GRIDOFF);
        }
        else if (key == key_map_mark)
        {
            M_snprintf(buffer, sizeof(buffer), "%s %d",
                       DEH_String(AMSTR_MARKEDSPOT), data->am_map.markpointnum);
			data->am_map.plr->message = buffer;
            AM_addMark(data);
        }
        else if (key == key_map_clearmark)
        {
            AM_clearMarks(data);
			data->am_map.plr->message = DEH_String(AMSTR_MARKSCLEARED);
        }
        else
        {
            rc = false;
        }

	if (!deathmatch && cht_CheckCheat(&data->am_map.cheat_amap, ev->data2))
	{
	    rc = false;
		data->am_map.cheating = (data->am_map.cheating+1) % 3;
	}
    }
    else if (ev->type == ev_keyup)
    {
        rc = false;
        key = ev->data1;

        if (key == key_map_east)
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.x = 0;
        }
        else if (key == key_map_west)
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.x = 0;
        }
        else if (key == key_map_north)
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.y = 0;
        }
        else if (key == key_map_south)
        {
            if (!data->am_map.followplayer) data->am_map.m_paninc.y = 0;
        }
        else if (key == key_map_zoomout || key == key_map_zoomin)
        {
			data->am_map.mtof_zoommul = FRACUNIT;
			data->am_map.ftom_zoommul = FRACUNIT;
        }
    }

    return rc;

}


//
// Zooming
//
void AM_changeWindowScale(data_t* data)
{

    // Change the scaling multipliers
	data->am_map.scale_mtof = FixedMul(data->am_map.scale_mtof, data->am_map.mtof_zoommul);
	data->am_map.scale_ftom = FixedDiv(FRACUNIT, data->am_map.scale_mtof);

    if (data->am_map.scale_mtof < data->am_map.min_scale_mtof)
		AM_minOutWindowScale(data);
    else if (data->am_map.scale_mtof > data->am_map.max_scale_mtof)
		AM_maxOutWindowScale(data);
    else
		AM_activateNewScale(data);
}


//
//
//
void AM_doFollowPlayer(data_t* data)
{

    if (data->am_map.f_oldloc.x != data->am_map.plr->mo->x || data->am_map.f_oldloc.y != data->am_map.plr->mo->y)
    {
		data->am_map.m_x = FTOM(MTOF(data->am_map.plr->mo->x)) - data->am_map.m_w/2;
		data->am_map.m_y = FTOM(MTOF(data->am_map.plr->mo->y)) - data->am_map.m_h/2;
		data->am_map.m_x2 = data->am_map.m_x + data->am_map.m_w;
		data->am_map.m_y2 = data->am_map.m_y + data->am_map.m_h;
		data->am_map.f_oldloc.x = data->am_map.plr->mo->x;
		data->am_map.f_oldloc.y = data->am_map.plr->mo->y;

	//  m_x = FTOM(MTOF(plr->mo->x - m_w/2));
	//  m_y = FTOM(MTOF(plr->mo->y - m_h/2));
	//  m_x = plr->mo->x - m_w/2;
	//  m_y = plr->mo->y - m_h/2;

    }

}

//
//
//
void AM_updateLightLev(data_t* data)
{
    static int nexttic = 0;
    //static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
    static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };
    static int litelevelscnt = 0;
   
    // Change light level
    if (data->am_map.amclock>nexttic)
    {
		data->am_map.lightlev = litelevels[litelevelscnt++];
	if (litelevelscnt == arrlen(litelevels)) litelevelscnt = 0;
	nexttic = data->am_map.amclock + 6 - (data->am_map.amclock % 6);
    }

}


//
// Updates on Game Tick
//
void AM_Ticker (data_t* data)
{
    if (!data->am_map.automapactive)
		return;

	data->am_map.amclock++;

    if (data->am_map.followplayer)
		AM_doFollowPlayer(data);

    // Change the zoom if necessary
    if (data->am_map.ftom_zoommul != FRACUNIT)
		AM_changeWindowScale(data);

    // Change x,y location
    if (data->am_map.m_paninc.x || data->am_map.m_paninc.y)
		AM_changeWindowLoc(data);

    // Update light level
    // AM_updateLightLev();

}


//
// Clear automap frame buffer.
//
void AM_clearFB(data_t* data, int color)
{
    memset(data->am_map.fb, color, data->am_map.f_w * data->am_map.f_h);
}


//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
boolean
AM_clipMline
( data_t* data,
  mline_t*	ml,
  fline_t*	fl )
{
    enum
    {
	LEFT	=1,
	RIGHT	=2,
	BOTTOM	=4,
	TOP	=8
    };
    
    register int	outcode1 = 0;
    register int	outcode2 = 0;
    register int	outside;
    
    fpoint_t	tmp;
    int		dx;
    int		dy;

    
#define DOOUTCODE(oc, mx, my) \
    (oc) = 0; \
    if ((my) < 0) (oc) |= TOP; \
    else if ((my) >= data->am_map.f_h) (oc) |= BOTTOM; \
    if ((mx) < 0) (oc) |= LEFT; \
    else if ((mx) >= data->am_map.f_w) (oc) |= RIGHT;

    
    // do trivial rejects and outcodes
    if (ml->a.y > data->am_map.m_y2)
		outcode1 = TOP;
    else if (ml->a.y < data->am_map.m_y)
		outcode1 = BOTTOM;

    if (ml->b.y > data->am_map.m_y2)
		outcode2 = TOP;
    else if (ml->b.y < data->am_map.m_y)
		outcode2 = BOTTOM;
    
    if (outcode1 & outcode2)
		return false; // trivially outside

    if (ml->a.x < data->am_map.m_x)
		outcode1 |= LEFT;
    else if (ml->a.x > data->am_map.m_x2)
		outcode1 |= RIGHT;
    
    if (ml->b.x < data->am_map.m_x)
		outcode2 |= LEFT;
    else if (ml->b.x > data->am_map.m_x2)
		outcode2 |= RIGHT;
    
    if (outcode1 & outcode2)
		return false; // trivially outside

    // transform to frame-buffer coordinates.
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
	return false;

    while (outcode1 | outcode2)
    {
	// may be partially inside box
	// find an outside point
	if (outcode1)
	    outside = outcode1;
	else
	    outside = outcode2;
	
	// clip to each side
	if (outside & TOP)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
	    tmp.y = 0;
	}
	else if (outside & BOTTOM)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y-data->am_map.f_h))/dy;
	    tmp.y = data->am_map.f_h-1;
	}
	else if (outside & RIGHT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(data->am_map.f_w-1 - fl->a.x))/dx;
	    tmp.x = data->am_map.f_w-1;
	}
	else if (outside & LEFT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
	    tmp.x = 0;
	}
        else
        {
            tmp.x = 0;
            tmp.y = 0;
        }

	if (outside == outcode1)
	{
	    fl->a = tmp;
	    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	}
	else
	{
	    fl->b = tmp;
	    DOOUTCODE(outcode2, fl->b.x, fl->b.y);
	}
	
	if (outcode1 & outcode2)
	    return false; // trivially outside
    }

    return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void
AM_drawFline
( data_t* data,
  fline_t*	fl,
  int		color )
{
    register int x;
    register int y;
    register int dx;
    register int dy;
    register int sx;
    register int sy;
    register int ax;
    register int ay;
    register int d;
    
    static int fuck = 0;

    // For debugging only
    if (      fl->a.x < 0 || fl->a.x >= data->am_map.f_w
	   || fl->a.y < 0 || fl->a.y >= data->am_map.f_h
	   || fl->b.x < 0 || fl->b.x >= data->am_map.f_w
	   || fl->b.y < 0 || fl->b.y >= data->am_map.f_h)
    {
        DEH_fprintf(stderr, "fuck %d \r", fuck++);
	return;
    }

#define PUTDOT(xx,yy,cc) data->am_map.fb[(yy)*data->am_map.f_w+(xx)]=(cc)

    dx = fl->b.x - fl->a.x;
    ax = 2 * (dx<0 ? -dx : dx);
    sx = dx<0 ? -1 : 1;

    dy = fl->b.y - fl->a.y;
    ay = 2 * (dy<0 ? -dy : dy);
    sy = dy<0 ? -1 : 1;

    x = fl->a.x;
    y = fl->a.y;

    if (ax > ay)
    {
	d = ay - ax/2;
	while (1)
	{
	    PUTDOT(x,y,color);
	    if (x == fl->b.x) return;
	    if (d>=0)
	    {
		y += sy;
		d -= ax;
	    }
	    x += sx;
	    d += ay;
	}
    }
    else
    {
	d = ax - ay/2;
	while (1)
	{
	    PUTDOT(x, y, color);
	    if (y == fl->b.y) return;
	    if (d >= 0)
	    {
		x += sx;
		d -= ay;
	    }
	    y += sy;
	    d += ax;
	}
    }
}


//
// Clip lines, draw visible part sof lines.
//
void
AM_drawMline
( data_t* data,
  mline_t*	ml,
  int		color )
{
    static fline_t fl;

    if (AM_clipMline (data, ml, &fl))
		AM_drawFline (data, &fl, color); // draws it on frame buffer using fb coords
}



//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
void AM_drawGrid(data_t* data, int color)
{
    fixed_t x, y;
    fixed_t start, end;
    mline_t ml;

    // Figure out start of vertical gridlines
    start = data->am_map.m_x;
    if ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
	start += (MAPBLOCKUNITS<<FRACBITS)
	    - ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
    end = data->am_map.m_x + data->am_map.m_w;

    // draw vertical gridlines
    ml.a.y = data->am_map.m_y;
    ml.b.y = data->am_map.m_y+data->am_map.m_h;
    for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
    {
		ml.a.x = x;
		ml.b.x = x;
		AM_drawMline(data, &ml, color);
    }

    // Figure out start of horizontal gridlines
    start = data->am_map.m_y;
    if ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
	start += (MAPBLOCKUNITS<<FRACBITS)
	    - ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
    end = data->am_map.m_y + data->am_map.m_h;

    // draw horizontal gridlines
    ml.a.x = data->am_map.m_x;
    ml.b.x = data->am_map.m_x + data->am_map.m_w;
    for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
    {
		ml.a.y = y;
		ml.b.y = y;
		AM_drawMline(data, &ml, color);
    }

}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls(data_t* data)
{
    int i;
    static mline_t l;

    for (i=0;i<numlines;i++)
    {
	l.a.x = lines[i].v1->x;
	l.a.y = lines[i].v1->y;
	l.b.x = lines[i].v2->x;
	l.b.y = lines[i].v2->y;
	if (data->am_map.cheating || (lines[i].flags & ML_MAPPED))
	{
	    if ((lines[i].flags & LINE_NEVERSEE) && !data->am_map.cheating)
		continue;
	    if (!lines[i].backsector)
	    {
			AM_drawMline(data, &l, WALLCOLORS+data->am_map.lightlev);
	    }
	    else
	    {
			if (lines[i].special == 39)
			{ // teleporters
				AM_drawMline(data, &l, WALLCOLORS+WALLRANGE/2);
			}
			else if (lines[i].flags & ML_SECRET) // secret door
			{
				if (data->am_map.cheating) AM_drawMline(data, &l, SECRETWALLCOLORS + data->am_map.lightlev);
				else AM_drawMline(data, &l, WALLCOLORS+data->am_map.lightlev);
			}
			else if (lines[i].backsector->floorheight
				   != lines[i].frontsector->floorheight) {
				AM_drawMline(data, &l, FDWALLCOLORS + data->am_map.lightlev); // floor level change
			}
			else if (lines[i].backsector->ceilingheight
				   != lines[i].frontsector->ceilingheight) {
				AM_drawMline(data, &l, CDWALLCOLORS+data->am_map.lightlev); // ceiling level change
			}
			else if (data->am_map.cheating) {
				AM_drawMline(data, &l, TSWALLCOLORS+data->am_map.lightlev);
			}
	    }
	}
	else if (data->am_map.plr->powers[pw_allmap])
	{
	    if (!(lines[i].flags & LINE_NEVERSEE)) AM_drawMline(data, &l, GRAYS+3);
	}
    }
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void
AM_rotate
( fixed_t*	x,
  fixed_t*	y,
  angle_t	a )
{
    fixed_t tmpx;

    tmpx =
	FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
	- FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);
    
    *y   =
	FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
	+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

    *x = tmpx;
}

void
AM_drawLineCharacter
( data_t*	data,
  mline_t*	lineguy,
  int		lineguylines,
  fixed_t	scale,
  angle_t	angle,
  int		color,
  fixed_t	x,
  fixed_t	y )
{
    int		i;
    mline_t	l;

    for (i=0;i<lineguylines;i++)
    {
	l.a.x = lineguy[i].a.x;
	l.a.y = lineguy[i].a.y;

	if (scale)
	{
	    l.a.x = FixedMul(scale, l.a.x);
	    l.a.y = FixedMul(scale, l.a.y);
	}

	if (angle)
	    AM_rotate(&l.a.x, &l.a.y, angle);

	l.a.x += x;
	l.a.y += y;

	l.b.x = lineguy[i].b.x;
	l.b.y = lineguy[i].b.y;

	if (scale)
	{
	    l.b.x = FixedMul(scale, l.b.x);
	    l.b.y = FixedMul(scale, l.b.y);
	}

	if (angle)
	    AM_rotate(&l.b.x, &l.b.y, angle);
	
	l.b.x += x;
	l.b.y += y;

	AM_drawMline(data, &l, color);
    }
}

void AM_drawPlayers(data_t* data)
{
    int		i;
    player_t*	p;
    static int 	their_colors[] = { GREENS, GRAYS, BROWNS, REDS };
    int		their_color = -1;
    int		color;

    if (!netgame)
    {
	if (data->am_map.cheating)
	    AM_drawLineCharacter
		(data, cheat_player_arrow, arrlen(cheat_player_arrow), 0,
		 data->am_map.plr->mo->angle, WHITE, data->am_map.plr->mo->x, data->am_map.plr->mo->y);
	else
	    AM_drawLineCharacter
		(data, player_arrow, arrlen(player_arrow), 0, data->am_map.plr->mo->angle,
		 WHITE, data->am_map.plr->mo->x, data->am_map.plr->mo->y);
	return;
    }

    for (i=0;i<MAXPLAYERS;i++)
    {
	their_color++;
	p = &players[i];

	if ( (deathmatch && !singledemo) && p != data->am_map.plr)
	    continue;

	if (!playeringame[i])
	    continue;

	if (p->powers[pw_invisibility])
	    color = 246; // *close* to black
	else
	    color = their_colors[their_color];
	
	AM_drawLineCharacter
	    (data, player_arrow, arrlen(player_arrow), 0, p->mo->angle,
	     color, p->mo->x, p->mo->y);
    }

}

void
AM_drawThings
( data_t* data,
  int	colors,
  int 	colorrange)
{
    int		i;
    mobj_t*	t;

    for (i=0;i<numsectors;i++)
    {
	t = sectors[i].thinglist;
	while (t)
	{
	    AM_drawLineCharacter
		(data, thintriangle_guy, arrlen(thintriangle_guy),
		 16<<FRACBITS, t->angle, colors+data->am_map.lightlev, t->x, t->y);
	    t = t->snext;
	}
    }
}

void AM_drawMarks(data_t* data)
{
    int i, fx, fy, w, h;

    for (i=0;i<AM_NUMMARKPOINTS;i++)
    {
	if (data->am_map.markpoints[i].x != -1)
	{
	    //      w = SHORT(marknums[i]->width);
	    //      h = SHORT(marknums[i]->height);
	    w = 5; // because something's wrong with the wad, i guess
	    h = 6; // because something's wrong with the wad, i guess
	    fx = CXMTOF(data->am_map.markpoints[i].x);
	    fy = CYMTOF(data->am_map.markpoints[i].y);
	    if (fx >= data->am_map.f_x && fx <= data->am_map.f_w - w && fy >= data->am_map.f_y && fy <= data->am_map.f_h - h)
			V_DrawPatch(fx, fy, data->am_map.marknums[i]);
	}
    }

}

void AM_drawCrosshair(data_t* data, int color)
{
	data->am_map.fb[(data->am_map.f_w*(data->am_map.f_h+1))/2] = color; // single point for now

}

void AM_Drawer (data_t* data)
{
    if (!data->am_map.automapactive) return;

    AM_clearFB(data, BACKGROUND);
    if (data->am_map.grid)
		AM_drawGrid(data, GRIDCOLORS);
    AM_drawWalls(data);
    AM_drawPlayers(data);
    if (data->am_map.cheating==2)
		AM_drawThings(data, THINGCOLORS, THINGRANGE);
    AM_drawCrosshair(data, XHAIRCOLORS);

    AM_drawMarks(data);

    V_MarkRect(data->am_map.f_x, data->am_map.f_y, data->am_map.f_w, data->am_map.f_h);

}
