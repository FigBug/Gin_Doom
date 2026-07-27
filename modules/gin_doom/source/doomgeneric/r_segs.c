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
//	All the clipping: columns, horizontal spans, sky columns.
//






#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"


// OPTIMIZE: closed two sided data->lines as single sided

// True if any of the data->segs data->textures might be visible.

// False if the back side is the same plane.



// angle to line origin

//
// regular wall
//










//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( data_t* data,
  drawseg_t*	ds,
  int		x1,
  int		x2 )
{
    unsigned	index;
    column_t*	col;
    int		lightnum;
    int		texnum;
    
    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;
    texnum = data->texturetranslation[curline->sidedef->midtexture];
	
    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+data->extralight;

    if (curline->v1->y == curline->v2->y)
	lightnum--;
    else if (curline->v1->x == curline->v2->x)
	lightnum++;

    if (lightnum < 0)		
	data->walllights = data->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	data->walllights = data->scalelight[LIGHTLEVELS-1];
    else
	data->walllights = data->scalelight[lightnum];

    data->maskedtexturecol = ds->maskedtexturecol;

    data->rw_scalestep = ds->scalestep;		
    data->spryscale = ds->scale1 + (x1 - ds->x1)*data->rw_scalestep;
    data->mfloorclip = ds->sprbottomclip;
    data->mceilingclip = ds->sprtopclip;
    
    // find positioning
    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	data->dc_texturemid = frontsector->floorheight > backsector->floorheight
	    ? frontsector->floorheight : backsector->floorheight;
	data->dc_texturemid = data->dc_texturemid + data->textureheight[texnum] - data->viewz;
    }
    else
    {
	data->dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
	    ? frontsector->ceilingheight : backsector->ceilingheight;
	data->dc_texturemid = data->dc_texturemid - data->viewz;
    }
    data->dc_texturemid += curline->sidedef->rowoffset;
			
    if (data->fixedcolormap)
	data->dc_colormap = data->fixedcolormap;
    
    // draw the columns
    for (data->dc_x = x1 ; data->dc_x <= x2 ; data->dc_x++)
    {
	// calculate lighting
	if (data->maskedtexturecol[data->dc_x] != SHRT_MAX)
	{
	    if (!data->fixedcolormap)
	    {
		index = data->spryscale>>LIGHTSCALESHIFT;

		if (index >=  MAXLIGHTSCALE )
		    index = MAXLIGHTSCALE-1;

		data->dc_colormap = data->walllights[index];
	    }
			
	    data->sprtopscreen = data->centeryfrac - FixedMul(data->dc_texturemid, data->spryscale);
	    data->dc_iscale = 0xffffffffu / (unsigned)data->spryscale;
	    
	    // draw the texture
	    col = (column_t *)( 
		(byte *)R_GetColumn(data, texnum,data->maskedtexturecol[data->dc_x]) -3);
			
	    R_DrawMaskedColumn (data, col);
	    data->maskedtexturecol[data->dc_x] = SHRT_MAX;
	}
	data->spryscale += data->rw_scalestep;
    }
	
}




//
// R_RenderSegLoop
// Draws zero, one, or two data->textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  data->textures.
// CALLED: CORE LOOPING ROUTINE.
//
#define HEIGHTBITS		12
#define HEIGHTUNIT		(1<<HEIGHTBITS)

void R_RenderSegLoop (data_t* data)
{
    angle_t		angle;
    unsigned		index;
    int			yl;
    int			yh;
    int			mid;
    fixed_t		texturecolumn;
    int			top;
    int			bottom;

    for ( ; data->rw_x < data->rw_stopx ; data->rw_x++)
    {
	// mark floor / ceiling areas
	yl = (data->topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

	// no space above wall?
	if (yl < data->ceilingclip[data->rw_x]+1)
	    yl = data->ceilingclip[data->rw_x]+1;
	
	if (data->markceiling)
	{
	    top = data->ceilingclip[data->rw_x]+1;
	    bottom = yl-1;

	    if (bottom >= data->floorclip[data->rw_x])
		bottom = data->floorclip[data->rw_x]-1;

	    if (top <= bottom)
	    {
		data->ceilingplane->top[data->rw_x] = top;
		data->ceilingplane->bottom[data->rw_x] = bottom;
	    }
	}
		
	yh = data->bottomfrac>>HEIGHTBITS;

	if (yh >= data->floorclip[data->rw_x])
	    yh = data->floorclip[data->rw_x]-1;

	if (data->markfloor)
	{
	    top = yh+1;
	    bottom = data->floorclip[data->rw_x]-1;
	    if (top <= data->ceilingclip[data->rw_x])
		top = data->ceilingclip[data->rw_x]+1;
	    if (top <= bottom)
	    {
		data->floorplane->top[data->rw_x] = top;
		data->floorplane->bottom[data->rw_x] = bottom;
	    }
	}
	
	// texturecolumn and lighting are independent of wall tiers
	if (data->segtextured)
	{
	    // calculate texture offset
	    angle = (data->rw_centerangle + data->xtoviewangle[data->rw_x])>>ANGLETOFINESHIFT;
	    texturecolumn = data->rw_offset-FixedMul(finetangent[angle],data->rw_distance);
	    texturecolumn >>= FRACBITS;
	    // calculate lighting
	    index = data->rw_scale>>LIGHTSCALESHIFT;

	    if (index >=  MAXLIGHTSCALE )
		index = MAXLIGHTSCALE-1;

	    data->dc_colormap = data->walllights[index];
	    data->dc_x = data->rw_x;
	    data->dc_iscale = 0xffffffffu / (unsigned)data->rw_scale;
	}
        else
        {
            // purely to shut up the compiler

            texturecolumn = 0;
        }
	
	// draw the wall tiers
	if (data->midtexture)
	{
	    // single sided line
	    data->dc_yl = yl;
	    data->dc_yh = yh;
	    data->dc_texturemid = data->rw_midtexturemid;
	    data->dc_source = R_GetColumn(data, data->midtexture,texturecolumn);
	    colfunc (data);
	    data->ceilingclip[data->rw_x] = data->viewheight;
	    data->floorclip[data->rw_x] = -1;
	}
	else
	{
	    // two sided line
	    if (data->toptexture)
	    {
		// top wall
		mid = data->pixhigh>>HEIGHTBITS;
		data->pixhigh += data->pixhighstep;

		if (mid >= data->floorclip[data->rw_x])
		    mid = data->floorclip[data->rw_x]-1;

		if (mid >= yl)
		{
		    data->dc_yl = yl;
		    data->dc_yh = mid;
		    data->dc_texturemid = data->rw_toptexturemid;
		    data->dc_source = R_GetColumn(data, data->toptexture,texturecolumn);
		    colfunc (data);
		    data->ceilingclip[data->rw_x] = mid;
		}
		else
		    data->ceilingclip[data->rw_x] = yl-1;
	    }
	    else
	    {
		// no top wall
		if (data->markceiling)
		    data->ceilingclip[data->rw_x] = yl-1;
	    }
			
	    if (data->bottomtexture)
	    {
		// bottom wall
		mid = (data->pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
		data->pixlow += data->pixlowstep;

		// no space above wall?
		if (mid <= data->ceilingclip[data->rw_x])
		    mid = data->ceilingclip[data->rw_x]+1;
		
		if (mid <= yh)
		{
		    data->dc_yl = mid;
		    data->dc_yh = yh;
		    data->dc_texturemid = data->rw_bottomtexturemid;
		    data->dc_source = R_GetColumn(data, data->bottomtexture,
					    texturecolumn);
		    colfunc (data);
		    data->floorclip[data->rw_x] = mid;
		}
		else
		    data->floorclip[data->rw_x] = yh+1;
	    }
	    else
	    {
		// no bottom wall
		if (data->markfloor)
		    data->floorclip[data->rw_x] = yh+1;
	    }
			
	    if (data->maskedtexture)
	    {
		// save texturecol
		//  for backdrawing of masked mid texture
		data->maskedtexturecol[data->rw_x] = texturecolumn;
	    }
	}
		
	data->rw_scale += data->rw_scalestep;
	data->topfrac += data->topstep;
	data->bottomfrac += data->bottomstep;
    }
}




//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void
R_StoreWallRange
( data_t* data,
  int	start,
  int	stop )
{
    fixed_t		hyp;
    fixed_t		sineval;
    angle_t		distangle, offsetangle;
    fixed_t		vtop;
    int			lightnum;

    // don't overflow and crash
    if (data->ds_p == &data->drawsegs[MAXDRAWSEGS])
	return;		
		
#ifdef RANGECHECK
    if (start >=data->viewwidth || start > stop)
	I_Error (NULL, "Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    sidedef = curline->sidedef;
    linedef = curline->linedef;

    // mark the segment as visible for auto map
    linedef->flags |= ML_MAPPED;
    
    // calculate data->rw_distance for scale calculation
    data->rw_normalangle = curline->angle + ANG90;
    offsetangle = abs(data->rw_normalangle-data->rw_angle1);
    
    if (offsetangle > ANG90)
	offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist(data, curline->v1->x, curline->v1->y);
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    data->rw_distance = FixedMul (hyp, sineval);
		
	
    data->ds_p->x1 = data->rw_x = start;
    data->ds_p->x2 = stop;
    data->ds_p->curline = curline;
    data->rw_stopx = stop+1;
    
    // calculate scale at both ends and step
    data->ds_p->scale1 = data->rw_scale = 
	R_ScaleFromGlobalAngle (data, data->viewangle + data->xtoviewangle[start]);
    
    if (stop > start )
    {
	data->ds_p->scale2 = R_ScaleFromGlobalAngle (data, data->viewangle + data->xtoviewangle[stop]);
	data->ds_p->scalestep = data->rw_scalestep = 
	    (data->ds_p->scale2 - data->rw_scale) / (stop-start);
    }
    else
    {
	// UNUSED: try to fix the stretched line bug
#if 0
	if (data->rw_distance < FRACUNIT/2)
	{
	    fixed_t		trx,try;
	    fixed_t		gxt,gyt;

	    trx = curline->v1->x - data->viewx;
	    try = curline->v1->y - data->viewy;
			
	    gxt = FixedMul(trx,data->viewcos); 
	    gyt = -FixedMul(try,data->viewsin); 
	    data->ds_p->scale1 = FixedDiv(data->projection, gxt-gyt)<<data->detailshift;
	}
#endif
	data->ds_p->scale2 = data->ds_p->scale1;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    data->worldtop = frontsector->ceilingheight - data->viewz;
    data->worldbottom = frontsector->floorheight - data->viewz;
	
    data->midtexture = data->toptexture = data->bottomtexture = data->maskedtexture = 0;
    data->ds_p->maskedtexturecol = NULL;
	
    if (!backsector)
    {
	// single sided line
	data->midtexture = data->texturetranslation[sidedef->midtexture];
	// a single sided line is terminal, so it must mark ends
	data->markfloor = data->markceiling = true;
	if (linedef->flags & ML_DONTPEGBOTTOM)
	{
	    vtop = frontsector->floorheight +
		data->textureheight[sidedef->midtexture];
	    // bottom of texture at bottom
	    data->rw_midtexturemid = vtop - data->viewz;	
	}
	else
	{
	    // top of texture at top
	    data->rw_midtexturemid = data->worldtop;
	}
	data->rw_midtexturemid += sidedef->rowoffset;

	data->ds_p->silhouette = SIL_BOTH;
	data->ds_p->sprtopclip = data->screenheightarray;
	data->ds_p->sprbottomclip = data->negonearray;
	data->ds_p->bsilheight = INT_MAX;
	data->ds_p->tsilheight = INT_MIN;
    }
    else
    {
	// two sided line
	data->ds_p->sprtopclip = data->ds_p->sprbottomclip = NULL;
	data->ds_p->silhouette = 0;
	
	if (frontsector->floorheight > backsector->floorheight)
	{
	    data->ds_p->silhouette = SIL_BOTTOM;
	    data->ds_p->bsilheight = frontsector->floorheight;
	}
	else if (backsector->floorheight > data->viewz)
	{
	    data->ds_p->silhouette = SIL_BOTTOM;
	    data->ds_p->bsilheight = INT_MAX;
	    // data->ds_p->sprbottomclip = data->negonearray;
	}
	
	if (frontsector->ceilingheight < backsector->ceilingheight)
	{
	    data->ds_p->silhouette |= SIL_TOP;
	    data->ds_p->tsilheight = frontsector->ceilingheight;
	}
	else if (backsector->ceilingheight < data->viewz)
	{
	    data->ds_p->silhouette |= SIL_TOP;
	    data->ds_p->tsilheight = INT_MIN;
	    // data->ds_p->sprtopclip = data->screenheightarray;
	}
		
	if (backsector->ceilingheight <= frontsector->floorheight)
	{
	    data->ds_p->sprbottomclip = data->negonearray;
	    data->ds_p->bsilheight = INT_MAX;
	    data->ds_p->silhouette |= SIL_BOTTOM;
	}
	
	if (backsector->floorheight >= frontsector->ceilingheight)
	{
	    data->ds_p->sprtopclip = data->screenheightarray;
	    data->ds_p->tsilheight = INT_MIN;
	    data->ds_p->silhouette |= SIL_TOP;
	}
	
	data->worldhigh = backsector->ceilingheight - data->viewz;
	data->worldlow = backsector->floorheight - data->viewz;
		
	// hack to allow height changes in outdoor areas
	if (frontsector->ceilingpic == data->skyflatnum 
	    && backsector->ceilingpic == data->skyflatnum)
	{
	    data->worldtop = data->worldhigh;
	}
	
			
	if (data->worldlow != data->worldbottom 
	    || backsector->floorpic != frontsector->floorpic
	    || backsector->lightlevel != frontsector->lightlevel)
	{
	    data->markfloor = true;
	}
	else
	{
	    // same plane on both data->sides
	    data->markfloor = false;
	}
	
			
	if (data->worldhigh != data->worldtop 
	    || backsector->ceilingpic != frontsector->ceilingpic
	    || backsector->lightlevel != frontsector->lightlevel)
	{
	    data->markceiling = true;
	}
	else
	{
	    // same plane on both data->sides
	    data->markceiling = false;
	}
	
	if (backsector->ceilingheight <= frontsector->floorheight
	    || backsector->floorheight >= frontsector->ceilingheight)
	{
	    // closed door
	    data->markceiling = data->markfloor = true;
	}
	

	if (data->worldhigh < data->worldtop)
	{
	    // top texture
	    data->toptexture = data->texturetranslation[sidedef->toptexture];
	    if (linedef->flags & ML_DONTPEGTOP)
	    {
		// top of texture at top
		data->rw_toptexturemid = data->worldtop;
	    }
	    else
	    {
		vtop =
		    backsector->ceilingheight
		    + data->textureheight[sidedef->toptexture];
		
		// bottom of texture
		data->rw_toptexturemid = vtop - data->viewz;	
	    }
	}
	if (data->worldlow > data->worldbottom)
	{
	    // bottom texture
	    data->bottomtexture = data->texturetranslation[sidedef->bottomtexture];

	    if (linedef->flags & ML_DONTPEGBOTTOM )
	    {
		// bottom of texture at bottom
		// top of texture at top
		data->rw_bottomtexturemid = data->worldtop;
	    }
	    else	// top of texture at top
		data->rw_bottomtexturemid = data->worldlow;
	}
	data->rw_toptexturemid += sidedef->rowoffset;
	data->rw_bottomtexturemid += sidedef->rowoffset;
	
	// allocate space for masked texture tables
	if (sidedef->midtexture)
	{
	    // masked data->midtexture
	    data->maskedtexture = true;
	    data->ds_p->maskedtexturecol = data->maskedtexturecol = data->lastopening - data->rw_x;
	    data->lastopening += data->rw_stopx - data->rw_x;
	}
    }
    
    // calculate data->rw_offset (only needed for textured data->lines)
    data->segtextured = data->midtexture | data->toptexture | data->bottomtexture | data->maskedtexture;

    if (data->segtextured)
    {
	offsetangle = data->rw_normalangle-data->rw_angle1;
	
	if (offsetangle > ANG180)
	    offsetangle = -offsetangle;

	if (offsetangle > ANG90)
	    offsetangle = ANG90;

	sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
	data->rw_offset = FixedMul (hyp, sineval);

	if (data->rw_normalangle-data->rw_angle1 < ANG180)
	    data->rw_offset = -data->rw_offset;

	data->rw_offset += sidedef->textureoffset + curline->offset;
	data->rw_centerangle = ANG90 + data->viewangle - data->rw_normalangle;
	
	// calculate light table
	//  use different light tables
	//  for horizontal / vertical / diagonal
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	if (!data->fixedcolormap)
	{
	    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+data->extralight;

	    if (curline->v1->y == curline->v2->y)
		lightnum--;
	    else if (curline->v1->x == curline->v2->x)
		lightnum++;

	    if (lightnum < 0)		
		data->walllights = data->scalelight[0];
	    else if (lightnum >= LIGHTLEVELS)
		data->walllights = data->scalelight[LIGHTLEVELS-1];
	    else
		data->walllights = data->scalelight[lightnum];
	}
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    
  
    if (frontsector->floorheight >= data->viewz)
    {
	// above view plane
	data->markfloor = false;
    }
    
    if (frontsector->ceilingheight <= data->viewz 
	&& frontsector->ceilingpic != data->skyflatnum)
    {
	// below view plane
	data->markceiling = false;
    }

    
    // calculate incremental stepping values for texture edges
    data->worldtop >>= 4;
    data->worldbottom >>= 4;
	
    data->topstep = -FixedMul (data->rw_scalestep, data->worldtop);
    data->topfrac = (data->centeryfrac>>4) - FixedMul (data->worldtop, data->rw_scale);

    data->bottomstep = -FixedMul (data->rw_scalestep,data->worldbottom);
    data->bottomfrac = (data->centeryfrac>>4) - FixedMul (data->worldbottom, data->rw_scale);
	
    if (backsector)
    {	
	data->worldhigh >>= 4;
	data->worldlow >>= 4;

	if (data->worldhigh < data->worldtop)
	{
	    data->pixhigh = (data->centeryfrac>>4) - FixedMul (data->worldhigh, data->rw_scale);
	    data->pixhighstep = -FixedMul (data->rw_scalestep,data->worldhigh);
	}
	
	if (data->worldlow > data->worldbottom)
	{
	    data->pixlow = (data->centeryfrac>>4) - FixedMul (data->worldlow, data->rw_scale);
	    data->pixlowstep = -FixedMul (data->rw_scalestep,data->worldlow);
	}
    }
    
    // render it
    if (data->markceiling)
	data->ceilingplane = R_CheckPlane (data, data->ceilingplane, data->rw_x, data->rw_stopx-1);
    
    if (data->markfloor)
	data->floorplane = R_CheckPlane (data, data->floorplane, data->rw_x, data->rw_stopx-1);

    R_RenderSegLoop (data);

    
    // save sprite clipping info
    if ( ((data->ds_p->silhouette & SIL_TOP) || data->maskedtexture)
	 && !data->ds_p->sprtopclip)
    {
	memcpy (data->lastopening, data->ceilingclip+start, 2*(data->rw_stopx-start));
	data->ds_p->sprtopclip = data->lastopening - start;
	data->lastopening += data->rw_stopx - start;
    }
    
    if ( ((data->ds_p->silhouette & SIL_BOTTOM) || data->maskedtexture)
	 && !data->ds_p->sprbottomclip)
    {
	memcpy (data->lastopening, data->floorclip+start, 2*(data->rw_stopx-start));
	data->ds_p->sprbottomclip = data->lastopening - start;
	data->lastopening += data->rw_stopx - start;	
    }

    if (data->maskedtexture && !(data->ds_p->silhouette&SIL_TOP))
    {
	data->ds_p->silhouette |= SIL_TOP;
	data->ds_p->tsilheight = INT_MIN;
    }
    if (data->maskedtexture && !(data->ds_p->silhouette&SIL_BOTTOM))
    {
	data->ds_p->silhouette |= SIL_BOTTOM;
	data->ds_p->bsilheight = INT_MAX;
    }
    data->ds_p++;
}

