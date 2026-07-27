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
//	BSP traversal, handling of LineSegs for rendering.
//




#include "doomdef.h"

#include "m_bbox.h"

#include "i_system.h"

#include "r_main.h"
#include "r_plane.h"
#include "r_things.h"

// State.
#include "doomstat.h"
#include "r_state.h"

//#include "r_local.h"






void
R_StoreWallRange
( data_t* data,
  int	start,
  int	stop );




//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (data_t* data)
{
    data->ds_p = data->drawsegs;
}



//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//


#define MAXSEGS		32

// data->newend is one past the last valid seg




//
// R_ClipSolidWallSegment
// Does handle solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
// 
void
R_ClipSolidWallSegment
( data_t* data,
  int			first,
  int			last )
{
    cliprange_t*	next;
    cliprange_t*	start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = data->solidsegs;
    while (start->last < first-1)
	start++;

    if (first < start->first)
    {
	if (last < start->first-1)
	{
	    // Post is entirely visible (above start),
	    //  so insert a new clippost.
	    R_StoreWallRange (data, first, last);
	    next = data->newend;
	    data->newend++;
	    
	    while (next != start)
	    {
		*next = *(next-1);
		next--;
	    }
	    next->first = first;
	    next->last = last;
	    return;
	}
		
	// There is a fragment above *start.
	R_StoreWallRange (data, first, start->first - 1);
	// Now adjust the clip size.
	start->first = first;	
    }

    // Bottom contained in start?
    if (last <= start->last)
	return;			
		
    next = start;
    while (last >= (next+1)->first-1)
    {
	// There is a fragment between two posts.
	R_StoreWallRange (data, next->last + 1, (next+1)->first - 1);
	next++;
	
	if (last <= next->last)
	{
	    // Bottom is contained in next.
	    // Adjust the clip size.
	    start->last = next->last;	
	    goto crunch;
	}
    }
	
    // There is a fragment after *next.
    R_StoreWallRange (data, next->last + 1, last);
    // Adjust the clip size.
    start->last = last;
	
    // Remove start+1 to next from the clip list,
    // because start now covers their area.
  crunch:
    if (next == start)
    {
	// Post just extended past the bottom of one post.
	return;
    }
    

    while (next++ != data->newend)
    {
	// Remove a post.
	*++start = *next;
    }

    data->newend = start+1;
}



//
// R_ClipPassWallSegment
// Clips the given range of columns,
//  but does not includes it in the clip list.
// Does handle windows,
//  e.g. LineDefs with upper and lower texture.
//
void
R_ClipPassWallSegment
( data_t* data,
  int	first,
  int	last )
{
    cliprange_t*	start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = data->solidsegs;
    while (start->last < first-1)
	start++;

    if (first < start->first)
    {
	if (last < start->first-1)
	{
	    // Post is entirely visible (above start).
	    R_StoreWallRange (data, first, last);
	    return;
	}
		
	// There is a fragment above *start.
	R_StoreWallRange (data, first, start->first - 1);
    }

    // Bottom contained in start?
    if (last <= start->last)
	return;			
		
    while (last >= (start+1)->first-1)
    {
	// There is a fragment between two posts.
	R_StoreWallRange (data, start->last + 1, (start+1)->first - 1);
	start++;
	
	if (last <= start->last)
	    return;
    }
	
    // There is a fragment after *next.
    R_StoreWallRange (data, start->last + 1, last);
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (data_t* data)
{
    data->solidsegs[0].first = -0x7fffffff;
    data->solidsegs[0].last = -1;
    data->solidsegs[1].first = data->viewwidth;
    data->solidsegs[1].last = 0x7fffffff;
    data->newend = data->solidsegs+2;
}

//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
void R_AddLine (data_t* data, seg_t* line)
{
    int			x1;
    int			x2;
    angle_t		angle1;
    angle_t		angle2;
    angle_t		span;
    angle_t		tspan;
    
    data->curline = line;

    // OPTIMIZE: quickly reject orthogonal back data->sides.
    angle1 = R_PointToAngle(data, line->v1->x, line->v1->y);
    angle2 = R_PointToAngle(data, line->v2->x, line->v2->y);
    
    // Clip to view edges.
    // OPTIMIZE: make constant out of 2*data->clipangle (FIELDOFVIEW).
    span = angle1 - angle2;
    
    // Back side? I.e. backface culling?
    if (span >= ANG180)
	return;		

    // Global angle needed by segcalc.
    data->rw_angle1 = angle1;
    angle1 -= data->viewangle;
    angle2 -= data->viewangle;
	
    tspan = angle1 + data->clipangle;
    if (tspan > 2*data->clipangle)
    {
	tspan -= 2*data->clipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return;
	
	angle1 = data->clipangle;
    }
    tspan = data->clipangle - angle2;
    if (tspan > 2*data->clipangle)
    {
	tspan -= 2*data->clipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return;	
	angle2 = -data->clipangle;
    }
    
    // The seg is in the view range,
    // but not necessarily visible.
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    x1 = data->viewangletox[angle1];
    x2 = data->viewangletox[angle2];

    // Does not cross a pixel?
    if (x1 == x2)
	return;				
	
    data->backsector = line->backsector;

    // Single sided line?
    if (!data->backsector)
	goto clipsolid;		

    // Closed door.
    if (data->backsector->ceilingheight <= data->frontsector->floorheight
	|| data->backsector->floorheight >= data->frontsector->ceilingheight)
	goto clipsolid;		

    // Window.
    if (data->backsector->ceilingheight != data->frontsector->ceilingheight
	|| data->backsector->floorheight != data->frontsector->floorheight)
	goto clippass;	
		
    // Reject empty data->lines used for triggers
    //  and special events.
    // Identical floor and ceiling on both data->sides,
    // identical light levels on both data->sides,
    // and no middle texture.
    if (data->backsector->ceilingpic == data->frontsector->ceilingpic
	&& data->backsector->floorpic == data->frontsector->floorpic
	&& data->backsector->lightlevel == data->frontsector->lightlevel
	&& data->curline->sidedef->midtexture == 0)
    {
	return;
    }
    
				
  clippass:
    R_ClipPassWallSegment (data, x1, x2-1);	
    return;
		
  clipsolid:
    R_ClipSolidWallSegment (data, x1, x2-1);
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
int	checkcoord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};


boolean R_CheckBBox (data_t* data, fixed_t* bspcoord)
{
    int			boxx;
    int			boxy;
    int			boxpos;

    fixed_t		x1;
    fixed_t		y1;
    fixed_t		x2;
    fixed_t		y2;
    
    angle_t		angle1;
    angle_t		angle2;
    angle_t		span;
    angle_t		tspan;
    
    cliprange_t*	start;

    int			sx1;
    int			sx2;
    
    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (data->viewx <= bspcoord[BOXLEFT])
	boxx = 0;
    else if (data->viewx < bspcoord[BOXRIGHT])
	boxx = 1;
    else
	boxx = 2;
		
    if (data->viewy >= bspcoord[BOXTOP])
	boxy = 0;
    else if (data->viewy > bspcoord[BOXBOTTOM])
	boxy = 1;
    else
	boxy = 2;
		
    boxpos = (boxy<<2)+boxx;
    if (boxpos == 5)
	return true;
	
    x1 = bspcoord[checkcoord[boxpos][0]];
    y1 = bspcoord[checkcoord[boxpos][1]];
    x2 = bspcoord[checkcoord[boxpos][2]];
    y2 = bspcoord[checkcoord[boxpos][3]];
    
    // check clip list for an open space
    angle1 = R_PointToAngle(data, x1, y1) - data->viewangle;
    angle2 = R_PointToAngle(data, x2, y2) - data->viewangle;
	
    span = angle1 - angle2;

    // Sitting on a line?
    if (span >= ANG180)
	return true;
    
    tspan = angle1 + data->clipangle;

    if (tspan > 2*data->clipangle)
    {
	tspan -= 2*data->clipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return false;	

	angle1 = data->clipangle;
    }
    tspan = data->clipangle - angle2;
    if (tspan > 2*data->clipangle)
    {
	tspan -= 2*data->clipangle;

	// Totally off the left edge?
	if (tspan >= span)
	    return false;
	
	angle2 = -data->clipangle;
    }


    // Find the first clippost
    //  that touches the source post
    //  (adjacent pixels are touching).
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    sx1 = data->viewangletox[angle1];
    sx2 = data->viewangletox[angle2];

    // Does not cross a pixel.
    if (sx1 == sx2)
	return false;			
    sx2--;
	
    start = data->solidsegs;
    while (start->last < sx2)
	start++;
    
    if (sx1 >= start->first
	&& sx2 <= start->last)
    {
	// The clippost contains the new span.
	return false;
    }

    return true;
}



//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (data_t* data, int num)
{
    int			count;
    seg_t*		line;
    subsector_t*	sub;
	
#ifdef RANGECHECK
    if (num>=data->numsubsectors)
	I_Error (NULL, "R_Subsector: ss %i with numss = %i",
		 num,
		 data->numsubsectors);
#endif

    data->sscount++;
    sub = &data->subsectors[num];
    data->frontsector = sub->sector;
    count = sub->numlines;
    line = &data->segs[sub->firstline];

    if (data->frontsector->floorheight < data->viewz)
    {
	data->floorplane = R_FindPlane (data, data->frontsector->floorheight,
				  data->frontsector->floorpic,
				  data->frontsector->lightlevel);
    }
    else
	data->floorplane = NULL;
    
    if (data->frontsector->ceilingheight > data->viewz 
	|| data->frontsector->ceilingpic == data->skyflatnum)
    {
	data->ceilingplane = R_FindPlane (data, data->frontsector->ceilingheight,
				    data->frontsector->ceilingpic,
				    data->frontsector->lightlevel);
    }
    else
	data->ceilingplane = NULL;
		
    R_AddSprites (data, data->frontsector);	

    while (count--)
    {
	R_AddLine (data, line);
	line++;
    }
}




//
// RenderBSPNode
// Renders all data->subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
void R_RenderBSPNode (data_t* data, int bspnum)
{
    node_t*	bsp;
    int		side;

    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
    {
	if (bspnum == -1)			
	    R_Subsector (data, 0);
	else
	    R_Subsector (data, bspnum&(~NF_SUBSECTOR));
	return;
    }
		
    bsp = &data->nodes[bspnum];
    
    // Decide which side the view point is on.
    side = R_PointOnSide (data->viewx, data->viewy, bsp);

    // Recursively divide front space.
    R_RenderBSPNode (data, bsp->children[side]);

    // Possibly divide back space.
    if (R_CheckBBox (data, bsp->bbox[side^1]))	
	R_RenderBSPNode (data, bsp->children[side^1]);
}


