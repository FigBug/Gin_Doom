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
//	Here is a core component: drawing the floors and ceilings,
//	 while maintaining a per column clipping list only.
//	Moreover, the sky areas have to be determined.
//


#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"



planefunction_t		floorfunc;
planefunction_t		ceilingfunc;

//
// opening
//

// Here comes the obnoxious "visplane".
#define MAXVISPLANES	128

// ?
#define MAXOPENINGS	SCREENWIDTH*64


//
// Clip values are the solid pixel bounding the range.
//  data->floorclip starts out SCREENHEIGHT
//  data->ceilingclip starts out -1
//

//
// data->spanstart holds the start of a plane span
// initialized to 0 at start
//

//
// texture mapping
//





//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
  // Doh!
}


//
// R_MapPlane
//
// Uses global vars:
//  data->planeheight
//  data->ds_source
//  data->basexscale
//  data->baseyscale
//  data->viewx
//  data->viewy
//
// BASIC PRIMITIVE
//
void
R_MapPlane
( data_t* data,
  int		y,
  int		x1,
  int		x2 )
{
    angle_t	angle;
    fixed_t	distance;
    fixed_t	length;
    unsigned	index;
	
#ifdef RANGECHECK
    if (x2 < x1
     || x1 < 0
     || x2 >= data->viewwidth
     || y > data->viewheight)
    {
	I_Error (NULL, "R_MapPlane: %i, %i at %i",x1,x2,y);
    }
#endif

    if (data->planeheight != data->cachedheight[y])
    {
	data->cachedheight[y] = data->planeheight;
	distance = data->cacheddistance[y] = FixedMul (data->planeheight, data->yslope[y]);
	data->ds_xstep = data->cachedxstep[y] = FixedMul (distance,data->basexscale);
	data->ds_ystep = data->cachedystep[y] = FixedMul (distance,data->baseyscale);
    }
    else
    {
	distance = data->cacheddistance[y];
	data->ds_xstep = data->cachedxstep[y];
	data->ds_ystep = data->cachedystep[y];
    }
	
    length = FixedMul (distance,data->distscale[x1]);
    angle = (data->viewangle + data->xtoviewangle[x1])>>ANGLETOFINESHIFT;
    data->ds_xfrac = data->viewx + FixedMul(finecosine[angle], length);
    data->ds_yfrac = -data->viewy - FixedMul(finesine[angle], length);

    if (data->fixedcolormap)
	data->ds_colormap = data->fixedcolormap;
    else
    {
	index = distance >> LIGHTZSHIFT;
	
	if (index >= MAXLIGHTZ )
	    index = MAXLIGHTZ-1;

	data->ds_colormap = data->planezlight[index];
    }
	
    data->ds_y = y;
    data->ds_x1 = x1;
    data->ds_x2 = x2;

    // high or low detail
    spanfunc (data);	
}


//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes (data_t* data)
{
    int		i;
    angle_t	angle;
    
    // opening / clipping determination
    for (i=0 ; i<data->viewwidth ; i++)
    {
	data->floorclip[i] = data->viewheight;
	data->ceilingclip[i] = -1;
    }

    data->lastvisplane = data->visplanes;
    data->lastopening = data->openings;
    
    // texture calculation
    memset (data->cachedheight, 0, sizeof(data->cachedheight));

    // left to right mapping
    angle = (data->viewangle-ANG90)>>ANGLETOFINESHIFT;
	
    // scale will be unit scale at SCREENWIDTH/2 distance
    data->basexscale = FixedDiv (finecosine[angle],data->centerxfrac);
    data->baseyscale = -FixedDiv (finesine[angle],data->centerxfrac);
}




//
// R_FindPlane
//
visplane_t*
R_FindPlane
( data_t* data,
  fixed_t	height,
  int		picnum,
  int		lightlevel )
{
    visplane_t*	check;
	
    if (picnum == data->skyflatnum)
    {
	height = 0;			// all skys map together
	lightlevel = 0;
    }
	
    for (check=data->visplanes; check<data->lastvisplane; check++)
    {
	if (height == check->height
	    && picnum == check->picnum
	    && lightlevel == check->lightlevel)
	{
	    break;
	}
    }
    
			
    if (check < data->lastvisplane)
	return check;
		
    if (data->lastvisplane - data->visplanes == MAXVISPLANES)
	I_Error (data, "R_FindPlane: no more data->visplanes");
		
    data->lastvisplane++;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;
    
    memset (check->top,0xff,sizeof(check->top));
		
    return check;
}


//
// R_CheckPlane
//
visplane_t*
R_CheckPlane
( data_t* data,
  visplane_t*	pl,
  int		start,
  int		stop )
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;
	
    if (start < pl->minx)
    {
	intrl = pl->minx;
	unionl = start;
    }
    else
    {
	unionl = pl->minx;
	intrl = start;
    }
	
    if (stop > pl->maxx)
    {
	intrh = pl->maxx;
	unionh = stop;
    }
    else
    {
	unionh = pl->maxx;
	intrh = stop;
    }

    for (x=intrl ; x<= intrh ; x++)
	if (pl->top[x] != 0xff)
	    break;

    if (x > intrh)
    {
	pl->minx = unionl;
	pl->maxx = unionh;

	// use the same one
	return pl;		
    }
	
    // make a new visplane
    data->lastvisplane->height = pl->height;
    data->lastvisplane->picnum = pl->picnum;
    data->lastvisplane->lightlevel = pl->lightlevel;
    
    pl = data->lastvisplane++;
    pl->minx = start;
    pl->maxx = stop;

    memset (pl->top,0xff,sizeof(pl->top));
		
    return pl;
}


//
// R_MakeSpans
//
void
R_MakeSpans
( data_t* data,
  int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 )
{
    while (t1 < t2 && t1<=b1)
    {
	R_MapPlane (data, t1,data->spanstart[t1],x-1);
	t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
	R_MapPlane (data, b1,data->spanstart[b1],x-1);
	b1--;
    }
	
    while (t2 < t1 && t2<=b2)
    {
	data->spanstart[t2] = x;
	t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
	data->spanstart[b2] = x;
	b2--;
    }
}



//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes (data_t* data)
{
    visplane_t*		pl;
    int			light;
    int			x;
    int			stop;
    int			angle;
    int                 lumpnum;
				
#ifdef RANGECHECK
    if (data->ds_p - data->drawsegs > MAXDRAWSEGS)
	I_Error (NULL, "R_DrawPlanes: data->drawsegs overflow (%i)",
		 data->ds_p - data->drawsegs);
    
    if (data->lastvisplane - data->visplanes > MAXVISPLANES)
	I_Error (NULL, "R_DrawPlanes: visplane overflow (%i)",
		 data->lastvisplane - data->visplanes);
    
    if (data->lastopening - data->openings > MAXOPENINGS)
	I_Error (NULL, "R_DrawPlanes: opening overflow (%i)",
		 data->lastopening - data->openings);
#endif

    for (pl = data->visplanes ; pl < data->lastvisplane ; pl++)
    {
	if (pl->minx > pl->maxx)
	    continue;

	
	// sky flat
	if (pl->picnum == data->skyflatnum)
	{
	    data->dc_iscale = data->pspriteiscale>>data->detailshift;
	    
	    // Sky is allways drawn full bright,
	    //  i.e. data->colormaps[0] is used.
	    // Because of this hack, sky is not affected
	    //  by INVUL inverse mapping.
	    data->dc_colormap = data->colormaps;
	    data->dc_texturemid = data->skytexturemid;
	    for (x=pl->minx ; x <= pl->maxx ; x++)
	    {
		data->dc_yl = pl->top[x];
		data->dc_yh = pl->bottom[x];

		if (data->dc_yl <= data->dc_yh)
		{
		    angle = (data->viewangle + data->xtoviewangle[x])>>ANGLETOSKYSHIFT;
		    data->dc_x = x;
		    data->dc_source = R_GetColumn(data, data->skytexture, angle);
		    colfunc (data);
		}
	    }
	    continue;
	}
	
	// regular flat
        lumpnum = data->firstflat + data->flattranslation[pl->picnum];
	data->ds_source = W_CacheLumpNum(lumpnum, PU_STATIC);
	
	data->planeheight = abs(pl->height-data->viewz);
	light = (pl->lightlevel >> LIGHTSEGSHIFT)+data->extralight;

	if (light >= LIGHTLEVELS)
	    light = LIGHTLEVELS-1;

	if (light < 0)
	    light = 0;

	data->planezlight = data->zlight[light];

	pl->top[pl->maxx+1] = 0xff;
	pl->top[pl->minx-1] = 0xff;
		
	stop = pl->maxx + 1;

	for (x=pl->minx ; x<= stop ; x++)
	{
	    R_MakeSpans(data, x,pl->top[x-1],
			pl->bottom[x-1],
			pl->top[x],
			pl->bottom[x]);
	}
	
        W_ReleaseLumpNum(lumpnum);
    }
}
