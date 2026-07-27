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
//	Refresh of things, i.e. objects represented by sprites.
//




#include <stdio.h>
#include <stdlib.h>


#include "deh_main.h"
#include "doomdef.h"

#include "i_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "doomstat.h"



#define MINZ				(FRACUNIT*4)
#define BASEYCENTER			100

//void R_DrawColumn (void);
//void R_DrawFuzzColumn (void);



typedef struct
{
    int		x1;
    int		x2;
	
    int		column;
    int		topclip;
    int		bottomclip;

} maskdraw_t;



//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//


// constant arrays
//  used for psprite clipping and initializing clipping


//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t*	sprites;

spriteframe_t	sprtemp[29];
char*		spritename;




//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void
R_InstallSpriteLump
( data_t* data,
  int		lump,
  unsigned	frame,
  unsigned	rotation,
  boolean	flipped )
{
    int		r;
	
    if (frame >= 29 || rotation > 8)
	I_Error (NULL, "R_InstallSpriteLump: "
		"Bad frame characters in lump %i", lump);
	
    if ((int)frame > data->maxframe)
	data->maxframe = frame;
		
    if (rotation == 0)
    {
	// the lump should be used for all rotations
	if (sprtemp[frame].rotate == false)
	    I_Error (NULL, "R_InitSprites: Sprite %s frame %c has "
		     "multip rot=0 lump", spritename, 'A'+frame);

	if (sprtemp[frame].rotate == true)
	    I_Error (NULL, "R_InitSprites: Sprite %s frame %c has rotations "
		     "and a rot=0 lump", spritename, 'A'+frame);
			
	sprtemp[frame].rotate = false;
	for (r=0 ; r<8 ; r++)
	{
	    sprtemp[frame].lump[r] = lump - data->firstspritelump;
	    sprtemp[frame].flip[r] = (byte)flipped;
	}
	return;
    }
	
    // the lump is only used for one rotation
    if (sprtemp[frame].rotate == false)
	I_Error (NULL, "R_InitSprites: Sprite %s frame %c has rotations "
		 "and a rot=0 lump", spritename, 'A'+frame);
		
    sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;		
    if (sprtemp[frame].lump[rotation] != -1)
	I_Error (NULL, "R_InitSprites: Sprite %s : %c : %c "
		 "has two lumps mapped to it",
		 spritename, 'A'+frame, '1'+rotation);
		
    sprtemp[frame].lump[rotation] = lump - data->firstspritelump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}




//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant. 
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSpriteDefs (data_t* data, char** namelist) 
{ 
    char**	check;
    int		i;
    int		l;
    int		frame;
    int		rotation;
    int		start;
    int		end;
    int		patched;
		
    // count the number of sprite names
    check = namelist;
    while (*check != NULL)
	check++;

    data->numsprites = check-namelist;
	
    if (!data->numsprites)
	return;
		
    sprites = Z_Malloc(data, data->numsprites *sizeof(*sprites), PU_STATIC, NULL);
	
    start = data->firstspritelump-1;
    end = data->lastspritelump+1;
	
    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    // Just compare 4 characters as ints
    for (i=0 ; i<data->numsprites ; i++)
    {
	spritename = DEH_String(namelist[i]);
	memset (sprtemp,-1, sizeof(sprtemp));
		
	data->maxframe = -1;
	
	// scan the lumps,
	//  filling in the frames for whatever is found
	for (l=start+1 ; l<end ; l++)
	{
	    if (!strncasecmp(data->lumpinfo[l].name, spritename, 4))
	    {
		frame = data->lumpinfo[l].name[4] - 'A';
		rotation = data->lumpinfo[l].name[5] - '0';

		if (data->modifiedgame)
		    patched = W_GetNumForName(data, data->lumpinfo[l].name);
		else
		    patched = l;

		R_InstallSpriteLump (data, patched, frame, rotation, false);

		if (data->lumpinfo[l].name[6])
		{
		    frame = data->lumpinfo[l].name[6] - 'A';
		    rotation = data->lumpinfo[l].name[7] - '0';
		    R_InstallSpriteLump (data, l, frame, rotation, true);
		}
	    }
	}
	
	// check the frames that were found for completeness
	if (data->maxframe == -1)
	{
	    sprites[i].numframes = 0;
	    continue;
	}
		
	data->maxframe++;
	
	for (frame = 0 ; frame < data->maxframe ; frame++)
	{
	    switch ((int)sprtemp[frame].rotate)
	    {
	      case -1:
		// no rotations were found for that frame at all
		I_Error (NULL, "R_InitSprites: No patches found "
			 "for %s frame %c", spritename, frame+'A');
		break;
		
	      case 0:
		// only the first rotation is needed
		break;
			
	      case 1:
		// must have all 8 frames
		for (rotation=0 ; rotation<8 ; rotation++)
		    if (sprtemp[frame].lump[rotation] == -1)
			I_Error (NULL, "R_InitSprites: Sprite %s frame %c "
				 "is missing rotations",
				 spritename, frame+'A');
		break;
	    }
	}
	
	// allocate space for the frames present and copy sprtemp to it
	sprites[i].numframes = data->maxframe;
	sprites[i].spriteframes = 
	    Z_Malloc(data, data->maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy (sprites[i].spriteframes, sprtemp, data->maxframe*sizeof(spriteframe_t));
    }

}




//
// GAME FUNCTIONS
//



//
// R_InitSprites
// Called at program start.
//
void R_InitSprites (data_t* data, char** namelist)
{
    int		i;
	
    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	data->negonearray[i] = -1;
    }
	
    R_InitSpriteDefs (data, namelist);
}



//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites (data_t* data)
{
    data->vissprite_p = data->vissprites;
}


//
// R_NewVisSprite
//

vissprite_t* R_NewVisSprite (data_t* data)
{
    if (data->vissprite_p == &data->vissprites[MAXVISSPRITES])
	return &data->overflowsprite;
    
    data->vissprite_p++;
    return data->vissprite_p-1;
}



//
// R_DrawMaskedColumn
// Used for sprites and masked mid data->textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//


void R_DrawMaskedColumn (data_t* data, column_t* column)
{
    int		topscreen;
    int 	bottomscreen;
    fixed_t	basetexturemid;
	
    basetexturemid = data->dc_texturemid;
	
    for ( ; column->topdelta != 0xff ; ) 
    {
	// calculate unclipped screen coordinates
	//  for post
	topscreen = data->sprtopscreen + data->spryscale*column->topdelta;
	bottomscreen = topscreen + data->spryscale*column->length;

	data->dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
	data->dc_yh = (bottomscreen-1)>>FRACBITS;
		
	if (data->dc_yh >= data->mfloorclip[data->dc_x])
	    data->dc_yh = data->mfloorclip[data->dc_x]-1;
	if (data->dc_yl <= data->mceilingclip[data->dc_x])
	    data->dc_yl = data->mceilingclip[data->dc_x]+1;

	if (data->dc_yl <= data->dc_yh)
	{
	    data->dc_source = (byte *)column + 3;
	    data->dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
	    // data->dc_source = (byte *)column + 3 - column->topdelta;

	    // Drawn by either R_DrawColumn
	    //  or (SHADOW) R_DrawFuzzColumn.
	    colfunc (data);	
	}
	column = (column_t *)(  (byte *)column + column->length + 4);
    }
	
    data->dc_texturemid = basetexturemid;
}



//
// R_DrawVisSprite
//  data->mfloorclip and data->mceilingclip should also be set.
//
void
R_DrawVisSprite
( data_t* data,
  vissprite_t*	vis,
  int			x1,
  int			x2 )
{
    column_t*		column;
    int			texturecolumn;
    fixed_t		frac;
    patch_t*		patch;
	
	
    patch = W_CacheLumpNum(data, vis->patch+data->firstspritelump, PU_CACHE);

    data->dc_colormap = vis->colormap;
    
    if (!data->dc_colormap)
    {
	// NULL colormap = shadow draw
	colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
	colfunc = transcolfunc;
	data->dc_translation = data->translationtables - 256 +
	    ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }
	
    data->dc_iscale = abs(vis->xiscale)>>data->detailshift;
    data->dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    data->spryscale = vis->scale;
    data->sprtopscreen = data->centeryfrac - FixedMul(data->dc_texturemid,data->spryscale);
	
    for (data->dc_x=vis->x1 ; data->dc_x<=vis->x2 ; data->dc_x++, frac += vis->xiscale)
    {
	texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
	if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
	    I_Error (NULL, "R_DrawSpriteRange: bad texturecolumn");
#endif
	column = (column_t *) ((byte *)patch +
			       LONG(patch->columnofs[texturecolumn]));
	R_DrawMaskedColumn (data, column);
    }

    colfunc = basecolfunc;
}



//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite (data_t* data, mobj_t* thing)
{
    fixed_t		tr_x;
    fixed_t		tr_y;
    
    fixed_t		gxt;
    fixed_t		gyt;
    
    fixed_t		tx;
    fixed_t		tz;

    fixed_t		xscale;
    
    int			x1;
    int			x2;

    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    
    unsigned		rot;
    boolean		flip;
    
    int			index;

    vissprite_t*	vis;
    
    angle_t		ang;
    fixed_t		iscale;
    
    // transform the origin point
    tr_x = thing->x - data->viewx;
    tr_y = thing->y - data->viewy;
	
    gxt = FixedMul(tr_x,data->viewcos); 
    gyt = -FixedMul(tr_y,data->viewsin);
    
    tz = gxt-gyt; 

    // thing is behind view plane?
    if (tz < MINZ)
	return;
    
    xscale = FixedDiv(data->projection, tz);
	
    gxt = -FixedMul(tr_x,data->viewsin); 
    gyt = FixedMul(tr_y,data->viewcos); 
    tx = -(gyt+gxt); 

    // too far off the side?
    if (abs(tx)>(tz<<2))
	return;
    
    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if ((unsigned int) thing->sprite >= (unsigned int) data->numsprites)
	I_Error (NULL, "R_ProjectSprite: invalid sprite number %i ",
		 thing->sprite);
#endif
    sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
	I_Error (NULL, "R_ProjectSprite: invalid sprite frame %i : %i ",
		 thing->sprite, thing->frame);
#endif
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
	// choose a different rotation based on player view
	ang = R_PointToAngle(data, thing->x, thing->y);
	rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
	lump = sprframe->lump[rot];
	flip = (boolean)sprframe->flip[rot];
    }
    else
    {
	// use single rotation for all views
	lump = sprframe->lump[0];
	flip = (boolean)sprframe->flip[0];
    }
    
    // calculate edges of the shape
    tx -= data->spriteoffset[lump];	
    x1 = (data->centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    // off the right side?
    if (x1 > data->viewwidth)
	return;
    
    tx +=  data->spritewidth[lump];
    x2 = ((data->centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
	return;
    
    // store information in a vissprite
    vis = R_NewVisSprite (data);
    vis->mobjflags = thing->flags;
    vis->scale = xscale<<data->detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = thing->z + data->spritetopoffset[lump];
    vis->texturemid = vis->gzt - data->viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= data->viewwidth ? data->viewwidth-1 : x2;	
    iscale = FixedDiv (FRACUNIT, xscale);

    if (flip)
    {
	vis->startfrac = data->spritewidth[lump]-1;
	vis->xiscale = -iscale;
    }
    else
    {
	vis->startfrac = 0;
	vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);
    vis->patch = lump;
    
    // get light level
    if (thing->flags & MF_SHADOW)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (data->fixedcolormap)
    {
	// fixed map
	vis->colormap = data->fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = data->colormaps;
    }
    
    else
    {
	// diminished light
	index = xscale>>(LIGHTSCALESHIFT-data->detailshift);

	if (index >= MAXLIGHTSCALE) 
	    index = MAXLIGHTSCALE-1;

	vis->colormap = data->spritelights[index];
    }	
}




//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites (data_t* data, sector_t* sec)
{
    mobj_t*		thing;
    int			lightnum;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  data->subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == data->validcount)
	return;		

    // Well, now it will be done.
    sec->validcount = data->validcount;
	
    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+data->extralight;

    if (lightnum < 0)		
	data->spritelights = data->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	data->spritelights = data->scalelight[LIGHTLEVELS-1];
    else
	data->spritelights = data->scalelight[lightnum];

    // Handle all things in sector.
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
	R_ProjectSprite (data, thing);
}


//
// R_DrawPSprite
//
void R_DrawPSprite (data_t* data, pspdef_t* psp)
{
    fixed_t		tx;
    int			x1;
    int			x2;
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    boolean		flip;
    vissprite_t*	vis;
    vissprite_t		avis;
    
    // decide which patch to use
#ifdef RANGECHECK
    if ( (unsigned)psp->state->sprite >= (unsigned int) data->numsprites)
	I_Error (NULL, "R_ProjectSprite: invalid sprite number %i ",
		 psp->state->sprite);
#endif
    sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
	I_Error (NULL, "R_ProjectSprite: invalid sprite frame %i : %i ",
		 psp->state->sprite, psp->state->frame);
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];
    
    // calculate edges of the shape
    tx = psp->sx-160*FRACUNIT;
	
    tx -= data->spriteoffset[lump];	
    x1 = (data->centerxfrac + FixedMul (tx,data->pspritescale) ) >>FRACBITS;

    // off the right side
    if (x1 > data->viewwidth)
	return;		

    tx +=  data->spritewidth[lump];
    x2 = ((data->centerxfrac + FixedMul (tx, data->pspritescale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
	return;
    
    // store information in a vissprite
    vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-data->spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= data->viewwidth ? data->viewwidth-1 : x2;	
    vis->scale = data->pspritescale<<data->detailshift;
    
    if (flip)
    {
	vis->xiscale = -data->pspriteiscale;
	vis->startfrac = data->spritewidth[lump]-1;
    }
    else
    {
	vis->xiscale = data->pspriteiscale;
	vis->startfrac = 0;
    }
    
    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);

    vis->patch = lump;

    if (data->viewplayer->powers[pw_invisibility] > 4*32
	|| data->viewplayer->powers[pw_invisibility] & 8)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (data->fixedcolormap)
    {
	// fixed color
	vis->colormap = data->fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = data->colormaps;
    }
    else
    {
	// local light
	vis->colormap = data->spritelights[MAXLIGHTSCALE-1];
    }
	
    R_DrawVisSprite (data, vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites (data_t* data)
{
    int		i;
    int		lightnum;
    pspdef_t*	psp;
    
    // get light level
    lightnum =
	(data->viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) 
	+data->extralight;

    if (lightnum < 0)		
	data->spritelights = data->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	data->spritelights = data->scalelight[LIGHTLEVELS-1];
    else
	data->spritelights = data->scalelight[lightnum];
    
    // clip to screen bounds
    data->mfloorclip = data->screenheightarray;
    data->mceilingclip = data->negonearray;
    
    // add all active psprites
    for (i=0, psp=data->viewplayer->psprites;
	 i<NUMPSPRITES;
	 i++,psp++)
    {
	if (psp->state)
	    R_DrawPSprite (data, psp);
    }
}




//
// R_SortVisSprites
//


void R_SortVisSprites (data_t* data)
{
    int			i;
    int			count;
    vissprite_t*	ds;
    vissprite_t*	best;
    vissprite_t		unsorted;
    fixed_t		bestscale;

    count = data->vissprite_p - data->vissprites;
	
    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
	return;
		
    for (ds=data->vissprites ; ds<data->vissprite_p ; ds++)
    {
	ds->next = ds+1;
	ds->prev = ds-1;
    }
    
    data->vissprites[0].prev = &unsorted;
    unsorted.next = &data->vissprites[0];
    (data->vissprite_p-1)->next = &unsorted;
    unsorted.prev = data->vissprite_p-1;
    
    // pull the data->vissprites out by scale

    data->vsprsortedhead.next = data->vsprsortedhead.prev = &data->vsprsortedhead;
    for (i=0 ; i<count ; i++)
    {
	bestscale = INT_MAX;
        best = unsorted.next;
	for (ds=unsorted.next ; ds!= &unsorted ; ds=ds->next)
	{
	    if (ds->scale < bestscale)
	    {
		bestscale = ds->scale;
		best = ds;
	    }
	}
	best->next->prev = best->prev;
	best->prev->next = best->next;
	best->next = &data->vsprsortedhead;
	best->prev = data->vsprsortedhead.prev;
	data->vsprsortedhead.prev->next = best;
	data->vsprsortedhead.prev = best;
    }
}



//
// R_DrawSprite
//
void R_DrawSprite (data_t* data, vissprite_t* spr)
{
    drawseg_t*		ds;
    int			x;
    int			r1;
    int			r2;
    fixed_t		scale;
    fixed_t		lowscale;
    int			silhouette;
		
    for (x = spr->x1 ; x<=spr->x2 ; x++)
	data->clipbot[x] = data->cliptop[x] = -2;
    
    // Scan data->drawsegs from end to start for obscuring data->segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds=data->ds_p-1 ; ds >= data->drawsegs ; ds--)
    {
	// determine if the drawseg obscures the sprite
	if (ds->x1 > spr->x2
	    || ds->x2 < spr->x1
	    || (!ds->silhouette
		&& !ds->maskedtexturecol) )
	{
	    // does not cover sprite
	    continue;
	}
			
	r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
	r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

	if (ds->scale1 > ds->scale2)
	{
	    lowscale = ds->scale2;
	    scale = ds->scale1;
	}
	else
	{
	    lowscale = ds->scale1;
	    scale = ds->scale2;
	}
		
	if (scale < spr->scale
	    || ( lowscale < spr->scale
		 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
	{
	    // masked mid texture?
	    if (ds->maskedtexturecol)	
		R_RenderMaskedSegRange (data, ds, r1, r2);
	    // seg is behind sprite
	    continue;			
	}

	
	// clip this piece of the sprite
	silhouette = ds->silhouette;
	
	if (spr->gz >= ds->bsilheight)
	    silhouette &= ~SIL_BOTTOM;

	if (spr->gzt <= ds->tsilheight)
	    silhouette &= ~SIL_TOP;
			
	if (silhouette == 1)
	{
	    // bottom sil
	    for (x=r1 ; x<=r2 ; x++)
		if (data->clipbot[x] == -2)
		    data->clipbot[x] = ds->sprbottomclip[x];
	}
	else if (silhouette == 2)
	{
	    // top sil
	    for (x=r1 ; x<=r2 ; x++)
		if (data->cliptop[x] == -2)
		    data->cliptop[x] = ds->sprtopclip[x];
	}
	else if (silhouette == 3)
	{
	    // both
	    for (x=r1 ; x<=r2 ; x++)
	    {
		if (data->clipbot[x] == -2)
		    data->clipbot[x] = ds->sprbottomclip[x];
		if (data->cliptop[x] == -2)
		    data->cliptop[x] = ds->sprtopclip[x];
	    }
	}
		
    }
    
    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	if (data->clipbot[x] == -2)		
	    data->clipbot[x] = data->viewheight;

	if (data->cliptop[x] == -2)
	    data->cliptop[x] = -1;
    }
		
    data->mfloorclip = data->clipbot;
    data->mceilingclip = data->cliptop;
    R_DrawVisSprite (data, spr, spr->x1, spr->x2);
}




//
// R_DrawMasked
//
void R_DrawMasked (data_t* data)
{
    vissprite_t*	spr;
    drawseg_t*		ds;
	
    R_SortVisSprites (data);

    if (data->vissprite_p > data->vissprites)
    {
	// draw all data->vissprites back to front
	for (spr = data->vsprsortedhead.next ;
	     spr != &data->vsprsortedhead ;
	     spr=spr->next)
	{
	    
	    R_DrawSprite (data, spr);
	}
    }
    
    // render any remaining masked mid data->textures
    for (ds=data->ds_p-1 ; ds >= data->drawsegs ; ds--)
	if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (data, ds, ds->x1, ds->x2);
    
    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!data->viewangleoffset)		
	R_DrawPlayerSprites (data);
}



