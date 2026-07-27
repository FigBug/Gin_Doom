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
//	Mission begin melt/wipe screen special effect.
//

#include <string.h>

#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"

#include "doomtype.h"

#include "f_wipe.h"

//
//                       SCREEN WIPE PACKAGE
//

// State (go, wipe_scr_start, wipe_scr_end, wipe_scr, wipe_y) lives in data_t.


void
wipe_shittyColMajorXform
( data_t*	data,
  short*	array,
  int		width,
  int		height )
{
    int		x;
    int		y;
    short*	dest;

    dest = (short*) Z_Malloc(data, width*height*2, PU_STATIC, 0);

    for(y=0;y<height;y++)
	for(x=0;x<width;x++)
	    dest[x*height+y] = array[y*width+x];

    memcpy(array, dest, width*height*2);

    Z_Free(data, dest);

}

int
wipe_initColorXForm
( data_t*	data,
  int	width,
  int	height,
  int	ticks )
{
    memcpy(data->wipe_scr, data->wipe_scr_start, width*height);
    return 0;
}

int
wipe_doColorXForm
( data_t*	data,
  int	width,
  int	height,
  int	ticks )
{
    boolean	changed;
    byte*	w;
    byte*	e;
    int		newval;

    changed = false;
    w = data->wipe_scr;
    e = data->wipe_scr_end;

    while (w!=data->wipe_scr+width*height)
    {
	if (*w != *e)
	{
	    if (*w > *e)
	    {
		newval = *w - ticks;
		if (newval < *e)
		    *w = *e;
		else
		    *w = newval;
		changed = true;
	    }
	    else if (*w < *e)
	    {
		newval = *w + ticks;
		if (newval > *e)
		    *w = *e;
		else
		    *w = newval;
		changed = true;
	    }
	}
	w++;
	e++;
    }

    return !changed;

}

int
wipe_exitColorXForm
( data_t*	data,
  int	width,
  int	height,
  int	ticks )
{
    return 0;
}


int
wipe_initMelt
( data_t*	data,
  int	width,
  int	height,
  int	ticks )
{
    int i, r;

    // copy start screen to main screen
    memcpy(data->wipe_scr, data->wipe_scr_start, width*height);

    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform(data, (short*)data->wipe_scr_start, width/2, height);
    wipe_shittyColMajorXform(data, (short*)data->wipe_scr_end, width/2, height);

    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    data->wipe_y = (int *) Z_Malloc(data, width*sizeof(int), PU_STATIC, 0);
    data->wipe_y[0] = -(M_Random (data)%16);
    for (i=1;i<width;i++)
    {
	r = (M_Random (data)%3) - 1;
	data->wipe_y[i] = data->wipe_y[i-1] + r;
	if (data->wipe_y[i] > 0) data->wipe_y[i] = 0;
	else if (data->wipe_y[i] == -16) data->wipe_y[i] = -15;
    }

    return 0;
}

int
wipe_doMelt
( data_t*	data,
  int	width,
  int	height,
  int	ticks )
{
    int		i;
    int		j;
    int		dy;
    int		idx;

    short*	s;
    short*	d;
    boolean	done = true;

    width/=2;

    while (ticks--)
    {
	for (i=0;i<width;i++)
	{
	    if (data->wipe_y[i]<0)
	    {
		data->wipe_y[i]++; done = false;
	    }
	    else if (data->wipe_y[i] < height)
	    {
		dy = (data->wipe_y[i] < 16) ? data->wipe_y[i]+1 : 8;
		if (data->wipe_y[i]+dy >= height) dy = height - data->wipe_y[i];
		s = &((short *)data->wipe_scr_end)[i*height+data->wipe_y[i]];
		d = &((short *)data->wipe_scr)[data->wipe_y[i]*width+i];
		idx = 0;
		for (j=dy;j;j--)
		{
		    d[idx] = *(s++);
		    idx += width;
		}
		data->wipe_y[i] += dy;
		s = &((short *)data->wipe_scr_start)[i*height];
		d = &((short *)data->wipe_scr)[data->wipe_y[i]*width+i];
		idx = 0;
		for (j=height-data->wipe_y[i];j;j--)
		{
		    d[idx] = *(s++);
		    idx += width;
		}
		done = false;
	    }
	}
    }

    return done;

}

int
wipe_exitMelt
( data_t*	data,
  int	width,
  int	height,
  int	ticks )
{
    Z_Free(data, data->wipe_y);
    Z_Free(data, data->wipe_scr_start);
    Z_Free(data, data->wipe_scr_end);
    return 0;
}

int
wipe_StartScreen
( data_t*	data,
  int	x,
  int	y,
  int	width,
  int	height )
{
    data->wipe_scr_start = Z_Malloc(data, SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    I_ReadScreen(data, data->wipe_scr_start);
    return 0;
}

int
wipe_EndScreen
( data_t*	data,
  int	x,
  int	y,
  int	width,
  int	height )
{
    data->wipe_scr_end = Z_Malloc(data, SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    I_ReadScreen(data, data->wipe_scr_end);
    V_DrawBlock(data, x, y, width, height, data->wipe_scr_start); // restore start scr.
    return 0;
}

int
wipe_ScreenWipe
( data_t*	data,
  int	wipeno,
  int	x,
  int	y,
  int	width,
  int	height,
  int	ticks )
{
    int rc;
    static int (*wipes[])(data_t*, int, int, int) =
    {
	wipe_initColorXForm, wipe_doColorXForm, wipe_exitColorXForm,
	wipe_initMelt, wipe_doMelt, wipe_exitMelt
    };

    // initial stuff
    if (!data->wipe_go)
    {
	data->wipe_go = 1;
	// wipe_scr = (byte *) Z_Malloc(data, width*height, PU_STATIC, 0); // DEBUG
	data->wipe_scr = data->I_VideoBuffer;
	(*wipes[wipeno*3])(data, width, height, ticks);
    }

    // do a piece of wipe-in
    V_MarkRect(data, 0, 0, width, height);
    rc = (*wipes[wipeno*3+1])(data, width, height, ticks);
    //  V_DrawBlock(data, x, y, 0, width, height, wipe_scr); // DEBUG

    // final stuff
    if (rc)
    {
	data->wipe_go = 0;
	(*wipes[wipeno*3+2])(data, width, height, ticks);
    }

    return !data->wipe_go;
}
