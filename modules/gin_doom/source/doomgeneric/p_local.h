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
//	Play functions, animation, global header.
//


#ifndef __P_LOCAL__
#define __P_LOCAL__

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

#define FLOATSPEED		(FRACUNIT*4)


#define MAXHEALTH		100
#define VIEWHEIGHT		(41*FRACUNIT)

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS	128
#define MAPBLOCKSIZE	(MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT	(FRACBITS+7)
#define MAPBMASK		(MAPBLOCKSIZE-1)
#define MAPBTOFRAC		(MAPBLOCKSHIFT-FRACBITS)


// player radius for movement checking
#define PLAYERRADIUS	16*FRACUNIT

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS		32*FRACUNIT

#define GRAVITY		FRACUNIT
#define MAXMOVE		(30*FRACUNIT)

#define USERANGE		(64*FRACUNIT)
#define MELEERANGE		(64*FRACUNIT)
#define MISSILERANGE	(32*64*FRACUNIT)

// follow a player exlusively for 3 seconds
#define	BASETHRESHOLD	 	100



//
// P_TICK
//

// both the head and tail of the thinker list
extern	thinker_t	thinkercap;	


void P_InitThinkers (void);
void P_AddThinker (thinker_t* thinker);
void P_RemoveThinker (thinker_t* thinker);


//
// P_PSPR
//
void P_SetupPsprites (data_t* data, player_t* curplayer);
void P_MovePsprites (data_t* data, player_t* curplayer);
void P_DropWeapon (data_t* data, player_t* player);


//
// P_USER
//
void	P_PlayerThink (data_t* data, player_t* player);


//
// P_MOBJ
//
#define ONFLOORZ		INT_MIN
#define ONCEILINGZ		INT_MAX

// Time interval for item respawning.
#define ITEMQUESIZE		128

extern mapthing_t	itemrespawnque[ITEMQUESIZE];
extern int		itemrespawntime[ITEMQUESIZE];
extern int		iquehead;
extern int		iquetail;


void P_RespawnSpecials (data_t* data);

mobj_t*
P_SpawnMobj
( data_t* data,
  fixed_t	x,
  fixed_t	y,
  fixed_t	z,
  mobjtype_t	type );

void 	P_RemoveMobj (data_t* data, mobj_t* th);
mobj_t* P_SubstNullMobj (data_t* data, mobj_t* th);
boolean	P_SetMobjState (data_t* data, mobj_t* mobj, statenum_t state);
void 	P_MobjThinker (data_t* data, mobj_t* mobj);

void	P_SpawnPuff (data_t* data, fixed_t x, fixed_t y, fixed_t z);
void 	P_SpawnBlood (data_t* data, fixed_t x, fixed_t y, fixed_t z, int damage);
mobj_t* P_SpawnMissile (data_t* data, mobj_t* source, mobj_t* dest, mobjtype_t type);
void	P_SpawnPlayerMissile (data_t* data, mobj_t* source, mobjtype_t type);


//
// P_ENEMY
//
void P_NoiseAlert (mobj_t* target, mobj_t* emmiter);


//
// P_MAPUTL
//
typedef struct
{
    fixed_t	x;
    fixed_t	y;
    fixed_t	dx;
    fixed_t	dy;
    
} divline_t;

typedef struct
{
    fixed_t	frac;		// along trace line
    boolean	isaline;
    union {
	mobj_t*	thing;
	line_t*	line;
    }			d;
} intercept_t;

// Extended MAXINTERCEPTS, to allow for intercepts overrun emulation.

#define MAXINTERCEPTS_ORIGINAL 128
#define MAXINTERCEPTS          (MAXINTERCEPTS_ORIGINAL + 61)

extern intercept_t	intercepts[MAXINTERCEPTS];
extern intercept_t*	intercept_p;

typedef boolean (*traverser_t) (data_t*, intercept_t *in);

fixed_t P_AproxDistance (fixed_t dx, fixed_t dy);
int 	P_PointOnLineSide (fixed_t x, fixed_t y, line_t* line);
int 	P_PointOnDivlineSide (fixed_t x, fixed_t y, divline_t* line);
void 	P_MakeDivline (line_t* li, divline_t* dl);
fixed_t P_InterceptVector (divline_t* v2, divline_t* v1);
int 	P_BoxOnLineSide (fixed_t* tmbox, line_t* ld);

extern fixed_t		opentop;
extern fixed_t 		openbottom;
extern fixed_t		openrange;
extern fixed_t		lowfloor;

void 	P_LineOpening (line_t* linedef);

boolean P_BlockLinesIterator (data_t* data, int x, int y, boolean(*func)(data_t*,line_t*) );
boolean P_BlockThingsIterator (data_t* data, int x, int y, boolean(*func)(data_t*,mobj_t*) );

#define PT_ADDLINES		1
#define PT_ADDTHINGS	2
#define PT_EARLYOUT		4

extern divline_t	trace;

boolean
P_PathTraverse
( data_t* data,
  fixed_t	x1,
  fixed_t	y1,
  fixed_t	x2,
  fixed_t	y2,
  int		flags,
  boolean	(*trav) (data_t*,intercept_t *));

void P_UnsetThingPosition (mobj_t* thing);
void P_SetThingPosition (mobj_t* thing);


//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern boolean		floatok;
extern fixed_t		tmfloorz;
extern fixed_t		tmceilingz;


extern	line_t*		ceilingline;

// fraggle: I have increased the size of this buffer.  In the original Doom,
// overrunning past this limit caused other bits of memory to be overwritten,
// affecting demo playback.  However, in doing so, the limit was still
// exceeded.  So we have to support more than 8 specials.
//
// We keep the original limit, to detect what variables in memory were
// overwritten (see SpechitOverrun())

#define MAXSPECIALCROSS 		20
#define MAXSPECIALCROSS_ORIGINAL	8

extern	line_t*	spechit[MAXSPECIALCROSS];
extern	int	numspechit;

boolean P_CheckPosition (data_t* data, mobj_t *thing, fixed_t x, fixed_t y);
boolean P_TryMove (data_t* data, mobj_t* thing, fixed_t x, fixed_t y);
boolean P_TeleportMove (data_t* data, mobj_t* thing, fixed_t x, fixed_t y);
void	P_SlideMove (data_t* data, mobj_t* mo);
boolean P_CheckSight (data_t* data, mobj_t* t1, mobj_t* t2);
void 	P_UseLines (data_t* data, player_t* player);

boolean P_ChangeSector (data_t* data, sector_t* sector, boolean crunch);

extern mobj_t*	linetarget;	// who got hit (or NULL)

fixed_t
P_AimLineAttack
( data_t* data,
  mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance );

void
P_LineAttack
( data_t* data,
  mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance,
  fixed_t	slope,
  int		damage );

void
P_RadiusAttack
( data_t* data,
  mobj_t*	spot,
  mobj_t*	source,
  int		damage );



//
// P_SETUP
//
extern byte*		rejectmatrix;	// for fast sight rejection
extern short*		blockmaplump;	// offsets in blockmap are from here
extern short*		blockmap;
extern int		bmapwidth;
extern int		bmapheight;	// in mapblocks
extern fixed_t		bmaporgx;
extern fixed_t		bmaporgy;	// origin of block map
extern mobj_t**		blocklinks;	// for thing chains



//
// P_INTER
//
extern int		maxammo[NUMAMMO];
extern int		clipammo[NUMAMMO];

void
P_TouchSpecialThing
( data_t*   data,
  mobj_t*	special,
  mobj_t*	toucher );

void
P_DamageMobj
( data_t* data,
  mobj_t*	target,
  mobj_t*	inflictor,
  mobj_t*	source,
  int		damage );


//
// P_SPEC
//
#include "p_spec.h"


#endif	// __P_LOCAL__
