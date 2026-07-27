//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard, Andrey Budko
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
//	Movement, collision handling.
//	Shooting and aiming.
//

#include <stdio.h>
#include <stdlib.h>

#include "deh_misc.h"

#include "m_bbox.h"
#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"
// Data.
#include "sounds.h"

// Spechit overrun magic value.
//
// This is the value used by PrBoom-plus.  I think the value below is 
// actually better and works with more demos.  However, I think
// it's better for the spechits emulation to be compatible with
// PrBoom-plus, at least so that the big spechits emulation list
// on Doomworld can also be used with Chocolate Doom.

#define DEFAULT_SPECHIT_MAGIC 0x01C09C98

// This is from a post by myk on the Doomworld forums, 
// outputted from entryway's spechit_magic generator for
// s205n546.lmp.  The _exact_ value of this isn't too
// important; as long as it is in the right general
// range, it will usually work.  Otherwise, we can use
// the generator (hacked doom2.exe) and provide it 
// with -data->spechit.

//#define DEFAULT_SPECHIT_MAGIC 0x84f968e8




// If "data->floatok" true, move would be ok
// if within "data->tmfloorz - data->tmceilingz".


// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls

// keep track of special data->lines as they are hit,
// but don't process them until the move is proven valid




//
// TELEPORT MOVE
// 

//
// PIT_StompThing
//
boolean PIT_StompThing (data_t* data, mobj_t* thing)
{
    fixed_t	blockdist;
		
    if (!(thing->flags & MF_SHOOTABLE) )
	return true;
		
    blockdist = thing->radius + data->tmthing->radius;
    
    if ( abs(thing->x - data->tmx) >= blockdist
	 || abs(thing->y - data->tmy) >= blockdist )
    {
	// didn't hit it
	return true;
    }
    
    // don't clip against self
    if (thing == data->tmthing)
	return true;
    
    // monsters don't stomp things except on boss level
    if ( !data->tmthing->player && data->gamemap != 30)
	return false;	
		
    P_DamageMobj (data, thing, data->tmthing, data->tmthing, 10000);
	
    return true;
}


//
// P_TeleportMove
//
boolean
P_TeleportMove
( data_t* data,
  mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    
    subsector_t*	newsubsec;
    
    // kill anything occupying the position
    data->tmthing = thing;
    data->tmflags = thing->flags;
	
    data->tmx = x;
    data->tmy = y;
	
    data->tmbbox[BOXTOP] = y + data->tmthing->radius;
    data->tmbbox[BOXBOTTOM] = y - data->tmthing->radius;
    data->tmbbox[BOXRIGHT] = x + data->tmthing->radius;
    data->tmbbox[BOXLEFT] = x - data->tmthing->radius;

    newsubsec = R_PointInSubsector(data, x,y);
    data->ceilingline = NULL;
    
    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any contacted data->lines the step closer together
    // will adjust them.
    data->tmfloorz = data->tmdropoffz = newsubsec->sector->floorheight;
    data->tmceilingz = newsubsec->sector->ceilingheight;
			
    data->validcount++;
    data->numspechit = 0;
    
    // stomp on any things contacted
    xl = (data->tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (data->tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (data->tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (data->tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockThingsIterator(data,bx,by,PIT_StompThing))
		return false;
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    thing->floorz = data->tmfloorz;
    thing->ceilingz = data->tmceilingz;	
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (data, thing);
	
    return true;
}


//
// MOVEMENT ITERATOR FUNCTIONS
//

static void SpechitOverrun(data_t* data, line_t *ld);

//
// PIT_CheckLine
// Adjusts data->tmfloorz and data->tmceilingz as data->lines are contacted
//
boolean PIT_CheckLine (data_t* data, line_t* ld)
{
    if (data->tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
	|| data->tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
	|| data->tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
	|| data->tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
	return true;

    if (P_BoxOnLineSide (data->tmbbox, ld) != -1)
	return true;
		
    // A line has been hit
    
    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special data->lines that are only 8 pixels apart
    // could be crossed in either order.
    
    if (!ld->backsector)
	return false;		// one sided line
		
    if (!(data->tmthing->flags & MF_MISSILE) )
    {
	if ( ld->flags & ML_BLOCKING )
	    return false;	// explicitly blocking everything

	if ( !data->tmthing->player && ld->flags & ML_BLOCKMONSTERS )
	    return false;	// block monsters only
    }

    // set openrange, opentop, openbottom
    P_LineOpening (ld);	
	
    // adjust floor / ceiling heights
    if (opentop < data->tmceilingz)
    {
	data->tmceilingz = opentop;
	data->ceilingline = ld;
    }

    if (openbottom > data->tmfloorz)
	data->tmfloorz = openbottom;	

    if (lowfloor < data->tmdropoffz)
	data->tmdropoffz = lowfloor;
		
    // if contacted a special line, add it to the list
    if (ld->special)
    {
        data->spechit[data->numspechit] = ld;
	data->numspechit++;

        // fraggle: spechits overrun emulation code from prboom-plus
        if (data->numspechit > MAXSPECIALCROSS_ORIGINAL)
        {
            SpechitOverrun(data, ld);
        }
    }

    return true;
}

//
// PIT_CheckThing
//
boolean PIT_CheckThing (data_t* data, mobj_t* thing)
{
    fixed_t		blockdist;
    boolean		solid;
    int			damage;
		
    if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE) ))
	return true;
    
    blockdist = thing->radius + data->tmthing->radius;

    if ( abs(thing->x - data->tmx) >= blockdist
	 || abs(thing->y - data->tmy) >= blockdist )
    {
	// didn't hit it
	return true;	
    }
    
    // don't clip against self
    if (thing == data->tmthing)
	return true;
    
    // check for skulls slamming into things
    if (data->tmthing->flags & MF_SKULLFLY)
    {
	damage = ((P_Random (data)%8)+1)*data->tmthing->info->damage;
	
	P_DamageMobj (data, thing, data->tmthing, data->tmthing, damage);
	
	data->tmthing->flags &= ~MF_SKULLFLY;
	data->tmthing->momx = data->tmthing->momy = data->tmthing->momz = 0;
	
	P_SetMobjState (data, data->tmthing, data->tmthing->info->spawnstate);
	
	return false;		// stop moving
    }

    
    // missiles can hit other things
    if (data->tmthing->flags & MF_MISSILE)
    {
	// see if it went over / under
	if (data->tmthing->z > thing->z + thing->height)
	    return true;		// overhead
	if (data->tmthing->z+data->tmthing->height < thing->z)
	    return true;		// underneath
		
	if (data->tmthing->target 
         && (data->tmthing->target->type == thing->type || 
	    (data->tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
	    (data->tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
	{
	    // Don't hit same species as originator.
	    if (thing == data->tmthing->target)
		return true;

            // sdh: Add deh_species_infighting here.  We can override the
            // "monsters of the same species cant hurt each other" behavior
            // through dehacked patches

	    if (thing->type != MT_PLAYER && !deh_species_infighting)
	    {
		// Explode, but do no damage.
		// Let data->players missile other data->players.
		return false;
	    }
	}
	
	if (! (thing->flags & MF_SHOOTABLE) )
	{
	    // didn't do any damage
	    return !(thing->flags & MF_SOLID);	
	}
	
	// damage / explode
	damage = ((P_Random (data)%8)+1)*data->tmthing->info->damage;
	P_DamageMobj (data, thing, data->tmthing, data->tmthing->target, damage);

	// don't traverse any more
	return false;				
    }
    
    // check for special pickup
    if (thing->flags & MF_SPECIAL)
    {
	solid = thing->flags&MF_SOLID;
	if (data->tmflags&MF_PICKUP)
	{
	    // can remove thing
	    P_TouchSpecialThing (data, thing, data->tmthing);
	}
	return !solid;
    }
	
    return !(thing->flags & MF_SOLID);
}


//
// MOVEMENT CLIPPING
//

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
// 
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid data->lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  data->tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//
boolean
P_CheckPosition
(
  data_t* data,
  mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    subsector_t*	newsubsec;

    data->tmthing = thing;
    data->tmflags = thing->flags;
	
    data->tmx = x;
    data->tmy = y;
	
    data->tmbbox[BOXTOP] = y + data->tmthing->radius;
    data->tmbbox[BOXBOTTOM] = y - data->tmthing->radius;
    data->tmbbox[BOXRIGHT] = x + data->tmthing->radius;
    data->tmbbox[BOXLEFT] = x - data->tmthing->radius;

    newsubsec = R_PointInSubsector(data, x,y);
    data->ceilingline = NULL;
    
    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any contacted data->lines the step closer together
    // will adjust them.
    data->tmfloorz = data->tmdropoffz = newsubsec->sector->floorheight;
    data->tmceilingz = newsubsec->sector->ceilingheight;
			
    data->validcount++;
    data->numspechit = 0;

    if ( data->tmflags & MF_NOCLIP )
	return true;
    
    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.
    xl = (data->tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (data->tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (data->tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (data->tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockThingsIterator(data, bx,by,PIT_CheckThing))
		return false;
    
    // check data->lines
    xl = (data->tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (data->tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (data->tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (data->tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockLinesIterator (data, bx,by,PIT_CheckLine))
		return false;

    return true;
}


//
// P_TryMove
// Attempt to move to a new position,
// crossing special data->lines unless MF_TELEPORT is set.
//
boolean
P_TryMove
( data_t* data,
  mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    fixed_t	oldx;
    fixed_t	oldy;
    int		side;
    int		oldside;
    line_t*	ld;

    data->floatok = false;
    if (!P_CheckPosition (data, thing, x, y))
	return false;		// solid wall or thing
    
    if ( !(thing->flags & MF_NOCLIP) )
    {
	if (data->tmceilingz - data->tmfloorz < thing->height)
	    return false;	// doesn't fit

	data->floatok = true;
	
	if ( !(thing->flags&MF_TELEPORT) 
	     &&data->tmceilingz - thing->z < thing->height)
	    return false;	// mobj must lower itself to fit

	if ( !(thing->flags&MF_TELEPORT)
	     && data->tmfloorz - thing->z > 24*FRACUNIT )
	    return false;	// too big a step up

	if ( !(thing->flags&(MF_DROPOFF|MF_FLOAT))
	     && data->tmfloorz - data->tmdropoffz > 24*FRACUNIT )
	    return false;	// don't stand over a dropoff
    }
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    oldx = thing->x;
    oldy = thing->y;
    thing->floorz = data->tmfloorz;
    thing->ceilingz = data->tmceilingz;	
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (data, thing);
    
    // if any special data->lines were hit, do the effect
    if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
    {
	while (data->numspechit--)
	{
	    // see if the line was crossed
	    ld = data->spechit[data->numspechit];
	    side = P_PointOnLineSide (thing->x, thing->y, ld);
	    oldside = P_PointOnLineSide (oldx, oldy, ld);
	    if (side != oldside)
	    {
		if (ld->special)
		    P_CrossSpecialLine (data, ld-data->lines, oldside, thing);
	    }
	}
    }

    return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
boolean P_ThingHeightClip (data_t* data, mobj_t* thing)
{
    boolean		onfloor;
	
    onfloor = (thing->z == thing->floorz);
	
    P_CheckPosition (data, thing, thing->x, thing->y);
    // what about stranding a monster partially off an edge?
	
    thing->floorz = data->tmfloorz;
    thing->ceilingz = data->tmceilingz;
	
    if (onfloor)
    {
	// walking monsters rise and fall with the floor
	thing->z = thing->floorz;
    }
    else
    {
	// don't adjust a floating monster unless forced to
	if (thing->z+thing->height > thing->ceilingz)
	    thing->z = thing->ceilingz - thing->height;
    }
	
    if (thing->ceilingz - thing->floorz < thing->height)
	return false;
		
    return true;
}



//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//






//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
void P_HitSlideLine (data_t* data, line_t* ld)
{
    int			side;

    angle_t		lineangle;
    angle_t		moveangle;
    angle_t		deltaangle;
    
    fixed_t		movelen;
    fixed_t		newlen;
	
	
    if (ld->slopetype == ST_HORIZONTAL)
    {
	data->tmymove = 0;
	return;
    }
    
    if (ld->slopetype == ST_VERTICAL)
    {
	data->tmxmove = 0;
	return;
    }
	
    side = P_PointOnLineSide (data->slidemo->x, data->slidemo->y, ld);
	
    lineangle = R_PointToAngle2(data, 0,0, ld->dx, ld->dy);

    if (side == 1)
	lineangle += ANG180;

    moveangle = R_PointToAngle2(data, 0,0, data->tmxmove, data->tmymove);
    deltaangle = moveangle-lineangle;

    if (deltaangle > ANG180)
	deltaangle += ANG180;
    //	I_Error (NULL, "SlideLine: ang>ANG180");

    lineangle >>= ANGLETOFINESHIFT;
    deltaangle >>= ANGLETOFINESHIFT;
	
    movelen = P_AproxDistance (data->tmxmove, data->tmymove);
    newlen = FixedMul (movelen, finecosine[deltaangle]);

    data->tmxmove = FixedMul (newlen, finecosine[lineangle]);	
    data->tmymove = FixedMul (newlen, finesine[lineangle]);	
}


//
// PTR_SlideTraverse
//
boolean PTR_SlideTraverse (data_t* data, intercept_t* in)
{
    line_t*	li;
	
    if (!in->isaline)
	I_Error (data, "PTR_SlideTraverse: not a line?");
		
    li = in->d.line;
    
    if ( ! (li->flags & ML_TWOSIDED) )
    {
	if (P_PointOnLineSide (data->slidemo->x, data->slidemo->y, li))
	{
	    // don't hit the back side
	    return true;		
	}
	goto isblocking;
    }

    // set openrange, opentop, openbottom
    P_LineOpening (li);
    
    if (openrange < data->slidemo->height)
	goto isblocking;		// doesn't fit
		
    if (opentop - data->slidemo->z < data->slidemo->height)
	goto isblocking;		// mobj is too high

    if (openbottom - data->slidemo->z > 24*FRACUNIT )
	goto isblocking;		// too big a step up

    // this line doesn't block movement
    return true;		
	
    // the line does block movement,
    // see if it is closer than best so far
  isblocking:		
    if (in->frac < data->bestslidefrac)
    {
	data->secondslidefrac = data->bestslidefrac;
	data->secondslideline = data->bestslideline;
	data->bestslidefrac = in->frac;
	data->bestslideline = li;
    }
	
    return false;	// stop
}



//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove (data_t* data, mobj_t* mo)
{
    fixed_t		leadx;
    fixed_t		leady;
    fixed_t		trailx;
    fixed_t		traily;
    fixed_t		newx;
    fixed_t		newy;
    int			hitcount;
		
    data->slidemo = mo;
    hitcount = 0;
    
  retry:
    if (++hitcount == 3)
	goto stairstep;		// don't loop forever

    
    // trace along the three leading corners
    if (mo->momx > 0)
    {
	leadx = mo->x + mo->radius;
	trailx = mo->x - mo->radius;
    }
    else
    {
	leadx = mo->x - mo->radius;
	trailx = mo->x + mo->radius;
    }
	
    if (mo->momy > 0)
    {
	leady = mo->y + mo->radius;
	traily = mo->y - mo->radius;
    }
    else
    {
	leady = mo->y - mo->radius;
	traily = mo->y + mo->radius;
    }
		
    data->bestslidefrac = FRACUNIT+1;
	
    P_PathTraverse ( data, leadx, leady, leadx+mo->momx, leady+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( data, trailx, leady, trailx+mo->momx, leady+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( data, leadx, traily, leadx+mo->momx, traily+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    
    // move up to the wall
    if (data->bestslidefrac == FRACUNIT+1)
    {
	// the move most have hit the middle, so stairstep
      stairstep:
	if (!P_TryMove (data, mo, mo->x, mo->y + mo->momy))
	    P_TryMove (data, mo, mo->x + mo->momx, mo->y);
	return;
    }

    // fudge a bit to make sure it doesn't hit
    data->bestslidefrac -= 0x800;	
    if (data->bestslidefrac > 0)
    {
	newx = FixedMul (mo->momx, data->bestslidefrac);
	newy = FixedMul (mo->momy, data->bestslidefrac);
	
	if (!P_TryMove (data, mo, mo->x+newx, mo->y+newy))
	    goto stairstep;
    }
    
    // Now continue along the wall.
    // First calculate remainder.
    data->bestslidefrac = FRACUNIT-(data->bestslidefrac+0x800);
    
    if (data->bestslidefrac > FRACUNIT)
	data->bestslidefrac = FRACUNIT;
    
    if (data->bestslidefrac <= 0)
	return;
    
    data->tmxmove = FixedMul (mo->momx, data->bestslidefrac);
    data->tmymove = FixedMul (mo->momy, data->bestslidefrac);

    P_HitSlideLine (data, data->bestslideline);	// clip the moves

    mo->momx = data->tmxmove;
    mo->momy = data->tmymove;
		
    if (!P_TryMove (data, mo, mo->x+data->tmxmove, mo->y+data->tmymove))
    {
	goto retry;
    }
}


//
// P_LineAttack
//

// Height if not aiming up or down
// ???: use slope for monsters?



// slopes to top and bottom of target
extern fixed_t	topslope;
extern fixed_t	bottomslope;	


//
// PTR_AimTraverse
// Sets linetaget and data->aimslope when a target is aimed at.
//
boolean
PTR_AimTraverse (data_t* data, intercept_t* in)
{
    line_t*		li;
    mobj_t*		th;
    fixed_t		slope;
    fixed_t		thingtopslope;
    fixed_t		thingbottomslope;
    fixed_t		dist;
		
    if (in->isaline)
    {
	li = in->d.line;
	
	if ( !(li->flags & ML_TWOSIDED) )
	    return false;		// stop
	
	// Crosses a two sided line.
	// A two sided line will restrict
	// the possible target ranges.
	P_LineOpening (li);
	
	if (openbottom >= opentop)
	    return false;		// stop
	
	dist = FixedMul (data->attackrange, in->frac);

        if (li->backsector == NULL
         || li->frontsector->floorheight != li->backsector->floorheight)
	{
	    slope = FixedDiv (openbottom - data->shootz , dist);
	    if (slope > bottomslope)
		bottomslope = slope;
	}
		
	if (li->backsector == NULL
         || li->frontsector->ceilingheight != li->backsector->ceilingheight)
	{
	    slope = FixedDiv (opentop - data->shootz , dist);
	    if (slope < topslope)
		topslope = slope;
	}
		
	if (topslope <= bottomslope)
	    return false;		// stop
			
	return true;			// shot continues
    }
    
    // shoot a thing
    th = in->d.thing;
    if (th == data->shootthing)
	return true;			// can't shoot self
    
    if (!(th->flags&MF_SHOOTABLE))
	return true;			// corpse or something

    // check angles to see if the thing can be aimed at
    dist = FixedMul (data->attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - data->shootz , dist);

    if (thingtopslope < bottomslope)
	return true;			// shot over the thing

    thingbottomslope = FixedDiv (th->z - data->shootz, dist);

    if (thingbottomslope > topslope)
	return true;			// shot under the thing
    
    // this thing can be hit!
    if (thingtopslope > topslope)
	thingtopslope = topslope;
    
    if (thingbottomslope < bottomslope)
	thingbottomslope = bottomslope;

    data->aimslope = (thingtopslope+thingbottomslope)/2;
    data->linetarget = th;

    return false;			// don't go any farther
}


//
// PTR_ShootTraverse
//
boolean PTR_ShootTraverse (data_t* data, intercept_t* in)
{
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;
    fixed_t		frac;
    
    line_t*		li;
    
    mobj_t*		th;

    fixed_t		slope;
    fixed_t		dist;
    fixed_t		thingtopslope;
    fixed_t		thingbottomslope;
		
    if (in->isaline)
    {
	li = in->d.line;
	
	if (li->special)
	    P_ShootSpecialLine (data, data->shootthing, li);

	if ( !(li->flags & ML_TWOSIDED) )
	    goto hitline;
	
	// crosses a two sided line
	P_LineOpening (li);
		
	dist = FixedMul (data->attackrange, in->frac);

        // e6y: emulation of missed back side on two-sided data->lines.
        // backsector can be NULL when emulating missing back side.

        if (li->backsector == NULL)
        {
            slope = FixedDiv (openbottom - data->shootz , dist);
            if (slope > data->aimslope)
                goto hitline;

            slope = FixedDiv (opentop - data->shootz , dist);
            if (slope < data->aimslope)
                goto hitline;
        }
        else
        {
            if (li->frontsector->floorheight != li->backsector->floorheight)
            {
                slope = FixedDiv (openbottom - data->shootz , dist);
                if (slope > data->aimslope)
                    goto hitline;
            }

            if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
            {
                slope = FixedDiv (opentop - data->shootz , dist);
                if (slope < data->aimslope)
                    goto hitline;
            }
        }

	// shot continues
	return true;
	
	
	// hit line
      hitline:
	// position a bit closer
	frac = in->frac - FixedDiv (4*FRACUNIT,data->attackrange);
	x = trace.x + FixedMul (trace.dx, frac);
	y = trace.y + FixedMul (trace.dy, frac);
	z = data->shootz + FixedMul (data->aimslope, FixedMul(frac, data->attackrange));

	if (li->frontsector->ceilingpic == skyflatnum)
	{
	    // don't shoot the sky!
	    if (z > li->frontsector->ceilingheight)
		return false;
	    
	    // it's a sky hack wall
	    if	(li->backsector && li->backsector->ceilingpic == skyflatnum)
		return false;		
	}

	// Spawn bullet puffs.
	P_SpawnPuff (data, x,y,z);
	
	// don't go any farther
	return false;	
    }
    
    // shoot a thing
    th = in->d.thing;
    if (th == data->shootthing)
	return true;		// can't shoot self
    
    if (!(th->flags&MF_SHOOTABLE))
	return true;		// corpse or something
		
    // check angles to see if the thing can be aimed at
    dist = FixedMul (data->attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - data->shootz , dist);

    if (thingtopslope < data->aimslope)
	return true;		// shot over the thing

    thingbottomslope = FixedDiv (th->z - data->shootz, dist);

    if (thingbottomslope > data->aimslope)
	return true;		// shot under the thing

    
    // hit thing
    // position a bit closer
    frac = in->frac - FixedDiv (10*FRACUNIT,data->attackrange);

    x = trace.x + FixedMul (trace.dx, frac);
    y = trace.y + FixedMul (trace.dy, frac);
    z = data->shootz + FixedMul (data->aimslope, FixedMul(frac, data->attackrange));

    // Spawn bullet puffs or blod spots,
    // depending on target type.
    if (in->d.thing->flags & MF_NOBLOOD)
	P_SpawnPuff (data, x,y,z);
    else
	P_SpawnBlood (data, x,y,z, data->la_damage);

    if (data->la_damage)
	P_DamageMobj (data, th, data->shootthing, data->shootthing, data->la_damage);

    // don't go any farther
    return false;
	
}


//
// P_AimLineAttack
//
fixed_t
P_AimLineAttack
( data_t* data,
  mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance )
{
    fixed_t	x2;
    fixed_t	y2;

    t1 = P_SubstNullMobj(data, t1);
	
    angle >>= ANGLETOFINESHIFT;
    data->shootthing = t1;
    
    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    data->shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;

    // can't shoot outside view angles
    topslope = 100*FRACUNIT/160;	
    bottomslope = -100*FRACUNIT/160;
    
    data->attackrange = distance;
    data->linetarget = NULL;
	
    P_PathTraverse ( data, t1->x, t1->y,
		     x2, y2,
		     PT_ADDLINES|PT_ADDTHINGS,
		     PTR_AimTraverse );
		
    if (data->linetarget)
	return data->aimslope;

    return 0;
}
 

//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave data->linetarget set.
//
void
P_LineAttack
( data_t* data,
  mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance,
  fixed_t	slope,
  int		damage )
{
    fixed_t	x2;
    fixed_t	y2;
	
    angle >>= ANGLETOFINESHIFT;
    data->shootthing = t1;
    data->la_damage = damage;
    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    data->shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;
    data->attackrange = distance;
    data->aimslope = slope;
		
    P_PathTraverse ( data, t1->x, t1->y,
		     x2, y2,
		     PT_ADDLINES|PT_ADDTHINGS,
		     PTR_ShootTraverse );
}
 


//
// USE LINES
//

boolean	PTR_UseTraverse (data_t* data, intercept_t* in)
{
    int		side;
	
    if (!in->d.line->special)
    {
	P_LineOpening (in->d.line);
	if (openrange <= 0)
	{
	    S_StartSound(data, data->usething, sfx_noway);
	    
	    // can't use through a wall
	    return false;	
	}
	// not a special line, but keep checking
	return true ;		
    }
	
    side = 0;
    if (P_PointOnLineSide (data->usething->x, data->usething->y, in->d.line) == 1)
	side = 1;
    
    //	return false;		// don't use back side
	
    P_UseSpecialLine (data, data->usething, in->d.line, side);

    // can't use for than one special line in a row
    return false;
}


//
// P_UseLines
// Looks for special data->lines in front of the player to activate.
//
void P_UseLines (data_t* data, player_t*	player)
{
    int		angle;
    fixed_t	x1;
    fixed_t	y1;
    fixed_t	x2;
    fixed_t	y2;
	
    data->usething = player->mo;
		
    angle = player->mo->angle >> ANGLETOFINESHIFT;

    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
    y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];
	
    P_PathTraverse ( data, x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
}


//
// RADIUS ATTACK
//


//
// PIT_RadiusAttack
// "data->bombsource" is the creature
// that caused the explosion at "data->bombspot".
//
boolean PIT_RadiusAttack (data_t* data, mobj_t* thing)
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	dist;
	
    if (!(thing->flags & MF_SHOOTABLE) )
	return true;

    // Boss spider and cyborg
    // take no damage from concussion.
    if (thing->type == MT_CYBORG
	|| thing->type == MT_SPIDER)
	return true;	
		
    dx = abs(thing->x - data->bombspot->x);
    dy = abs(thing->y - data->bombspot->y);
    
    dist = dx>dy ? dx : dy;
    dist = (dist - thing->radius) >> FRACBITS;

    if (dist < 0)
	dist = 0;

    if (dist >= data->bombdamage)
	return true;	// out of range

    if ( P_CheckSight (data, thing, data->bombspot) )
    {
	// must be in direct path
	P_DamageMobj (data, thing, data->bombspot, data->bombsource, data->bombdamage - dist);
    }
    
    return true;
}


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void
P_RadiusAttack
( data_t* data,
  mobj_t*	spot,
  mobj_t*	source,
  int		damage )
{
    int		x;
    int		y;
    
    int		xl;
    int		xh;
    int		yl;
    int		yh;
    
    fixed_t	dist;
	
    dist = (damage+MAXRADIUS)<<FRACBITS;
    yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
    data->bombspot = spot;
    data->bombsource = source;
    data->bombdamage = damage;
	
    for (y=yl ; y<=yh ; y++)
	for (x=xl ; x<=xh ; x++)
	    P_BlockThingsIterator (data, x, y, PIT_RadiusAttack );
}



//
// SECTOR HEIGHT CHANGING
// After modifying a data->sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//


//
// PIT_ChangeSector
//
boolean PIT_ChangeSector (data_t* data, mobj_t*	thing)
{
    mobj_t*	mo;
	
    if (P_ThingHeightClip (data, thing))
    {
	// keep checking
	return true;
    }
    

    // crunch bodies to giblets
    if (thing->health <= 0)
    {
	P_SetMobjState (data, thing, S_GIBS);

	thing->flags &= ~MF_SOLID;
	thing->height = 0;
	thing->radius = 0;

	// keep checking
	return true;		
    }

    // crunch dropped items
    if (thing->flags & MF_DROPPED)
    {
	P_RemoveMobj (data, thing);
	
	// keep checking
	return true;		
    }

    if (! (thing->flags & MF_SHOOTABLE) )
    {
	// assume it is bloody gibs or something
	return true;			
    }
    
    data->nofit = true;

    if (data->crushchange && !(data->leveltime&3) )
    {
	P_DamageMobj(data, thing,NULL,NULL,10);

	// spray blood in a random direction
	mo = P_SpawnMobj (data, thing->x,
			  thing->y,
			  thing->z + thing->height/2, MT_BLOOD);
	
	mo->momx = (P_Random (data) - P_Random (data))<<12;
	mo->momy = (P_Random (data) - P_Random (data))<<12;
    }

    // keep checking (crush other things)	
    return true;	
}



//
// P_ChangeSector
//
boolean
P_ChangeSector
( data_t* data,
  sector_t*	sector,
  boolean	crunch )
{
    int		x;
    int		y;
	
    data->nofit = false;
    data->crushchange = crunch;
	
    // re-check heights for all things near the moving sector
    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
	for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
	    P_BlockThingsIterator (data, x, y, PIT_ChangeSector);
	
	
    return data->nofit;
}

// Code to emulate the behavior of Vanilla Doom when encountering an overrun
// of the data->spechit array.  This is by Andrey Budko (e6y) and comes from his
// PrBoom plus port.  A big thanks to Andrey for this.

static void SpechitOverrun(data_t* data, line_t *ld)
{
    static unsigned int baseaddr = 0;
    unsigned int addr;
   
    if (baseaddr == 0)
    {
        int p;

        // This is the first time we have had an overrun.  Work out
        // what base address we are going to use.
        // Allow a data->spechit value to be specified on the command line.

        //!
        // @category compat
        // @arg <n>
        //
        // Use the specified magic value when emulating data->spechit overruns.
        //

        p = M_CheckParmWithArgs(data, "-data->spechit", 1);
        
        if (p > 0)
        {
            M_StrToInt(data->myargv[p+1], (int *) &baseaddr);
        }
        else
        {
            baseaddr = DEFAULT_SPECHIT_MAGIC;
        }
    }
    
    // Calculate address used in doom2.exe

    addr = baseaddr + (ld - data->lines) * 0x3E;

    switch(data->numspechit)
    {
        case 9: 
        case 10:
        case 11:
        case 12:
            data->tmbbox[data->numspechit-9] = addr;
            break;
        case 13: 
            data->crushchange = addr; 
            break;
        case 14: 
            data->nofit = addr; 
            break;
        default:
            fprintf(stderr, "SpechitOverrun: Warning: unable to emulate"
                            "an overrun where data->numspechit=%i\n",
                            data->numspechit);
            break;
    }
}

