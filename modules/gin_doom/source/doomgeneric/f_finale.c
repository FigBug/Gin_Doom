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
//	Game completion, final screen animation.
//


#include <stdio.h>
#include <ctype.h>

// Functions.
#include "deh_main.h"
#include "i_system.h"
#include "i_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"

// Data.
#include "d_main.h"
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"

typedef enum
{
    F_STAGE_TEXT,
    F_STAGE_ARTSCREEN,
    F_STAGE_CAST,
} finalestage_t;

// ?
//#include "doomstat.h"
//#include "r_local.h"
//#include "f_finale.h"

// Stage of animation:


#define	TEXTSPEED	3
#define	TEXTWAIT	250

typedef struct
{
    GameMission_t mission;
    int episode, level;
    char *background;
    char *text;
} textscreen_t;

static textscreen_t textscreens[] =
{
    { doom,      1, 8,  "FLOOR4_8",  E1TEXT},
    { doom,      2, 8,  "SFLR6_1",   E2TEXT},
    { doom,      3, 8,  "MFLR8_4",   E3TEXT},
    { doom,      4, 8,  "MFLR8_3",   E4TEXT},

    { doom2,     1, 6,  "SLIME16",   C1TEXT},
    { doom2,     1, 11, "RROCK14",   C2TEXT},
    { doom2,     1, 20, "RROCK07",   C3TEXT},
    { doom2,     1, 30, "RROCK17",   C4TEXT},
    { doom2,     1, 15, "RROCK13",   C5TEXT},
    { doom2,     1, 31, "RROCK19",   C6TEXT},

    { pack_tnt,  1, 6,  "SLIME16",   T1TEXT},
    { pack_tnt,  1, 11, "RROCK14",   T2TEXT},
    { pack_tnt,  1, 20, "RROCK07",   T3TEXT},
    { pack_tnt,  1, 30, "RROCK17",   T4TEXT},
    { pack_tnt,  1, 15, "RROCK13",   T5TEXT},
    { pack_tnt,  1, 31, "RROCK19",   T6TEXT},

    { pack_plut, 1, 6,  "SLIME16",   P1TEXT},
    { pack_plut, 1, 11, "RROCK14",   P2TEXT},
    { pack_plut, 1, 20, "RROCK07",   P3TEXT},
    { pack_plut, 1, 30, "RROCK17",   P4TEXT},
    { pack_plut, 1, 15, "RROCK13",   P5TEXT},
    { pack_plut, 1, 31, "RROCK19",   P6TEXT},
};


void	F_StartCast (data_t* data);
void	F_CastTicker (data_t* data);
boolean F_CastResponder (data_t* data, event_t *ev);
void	F_CastDrawer (data_t* data);

//
// F_StartFinale
//
void F_StartFinale (data_t* data)
{
    size_t i;

    data->gameaction = ga_nothing;
    data->gamestate = GS_FINALE;
    data->viewactive = false;
    data->automapactive = false;

    if (logical_gamemission == doom)
    {
        S_ChangeMusic(data, mus_victor, true);
    }
    else
    {
        S_ChangeMusic(data, mus_read_m, true);
    }

    // Find the right screen and set the text and background

    for (i=0; i<arrlen(textscreens); ++i)
    {
        textscreen_t *screen = &textscreens[i];

        // Hack for Chex Quest

        if (data->gameversion == exe_chex && screen->mission == doom)
        {
            screen->level = 5;
        }

        if (logical_gamemission == screen->mission
         && (logical_gamemission != doom || data->gameepisode == screen->episode)
         && data->gamemap == screen->level)
        {
            data->finaletext = screen->text;
            data->finaleflat = screen->background;
        }
    }

    // Do dehacked substitutions of strings
  
    data->finaletext = DEH_String(data->finaletext);
    data->finaleflat = DEH_String(data->finaleflat);
    
    data->finalestage = F_STAGE_TEXT;
    data->finalecount = 0;
	
}



boolean F_Responder (data_t* data, event_t *event)
{
    if (data->finalestage == F_STAGE_CAST)
	return F_CastResponder (data, event);
	
    return false;
}


//
// F_Ticker
//
void F_Ticker (data_t* data)
{
    size_t		i;
    
    // check for skipping
    if ( (data->gamemode == commercial)
      && ( data->finalecount > 50) )
    {
      // go on to the next level
      for (i=0 ; i<MAXPLAYERS ; i++)
	if (data->players[i].cmd.buttons)
	  break;
				
      if (i < MAXPLAYERS)
      {	
	if (data->gamemap == 30)
	  F_StartCast (data);
	else
	  data->gameaction = ga_worlddone;
      }
    }
    
    // advance animation
    data->finalecount++;
	
    if (data->finalestage == F_STAGE_CAST)
    {
	F_CastTicker (data);
	return;
    }
	
    if ( data->gamemode == commercial)
	return;
		
    if (data->finalestage == F_STAGE_TEXT
     && data->finalecount>strlen (data->finaletext)*TEXTSPEED + TEXTWAIT)
    {
	data->finalecount = 0;
	data->finalestage = F_STAGE_ARTSCREEN;
	data->wipegamestate = -1;		// force a wipe
	if (data->gameepisode == 3)
	    S_StartMusic(data, mus_bunny);
    }
}



//
// F_TextWrite
//

#include "hu_stuff.h"
extern	patch_t *hu_font[HU_FONTSIZE];


void F_TextWrite (data_t* data)
{
    byte*	src;
    byte*	dest;
    
    int		x,y,w;
    signed int	count;
    char*	ch;
    int		c;
    int		cx;
    int		cy;
    
    // erase the entire screen to a tiled background
    src = W_CacheLumpName ( data->finaleflat , PU_CACHE);
    dest = data->I_VideoBuffer;
	
    for (y=0 ; y<SCREENHEIGHT ; y++)
    {
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
	{
	    memcpy (dest, src+((y&63)<<6), 64);
	    dest += 64;
	}
	if (SCREENWIDTH&63)
	{
	    memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
	    dest += (SCREENWIDTH&63);
	}
    }

    V_MarkRect(data, 0, 0, SCREENWIDTH, SCREENHEIGHT);
    
    // draw some of the text onto the screen
    cx = 10;
    cy = 10;
    ch = data->finaletext;
	
    count = ((signed int) data->finalecount - 10) / TEXTSPEED;
    if (count < 0)
	count = 0;
    for ( ; count ; count-- )
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = 10;
	    cy += 11;
	    continue;
	}
		
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}
		
	w = SHORT (hu_font[c]->width);
	if (cx+w > SCREENWIDTH)
	    break;
	V_DrawPatch(data, cx, cy, hu_font[c]);
	cx+=w;
    }
	
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
    char		*name;
    mobjtype_t	type;
} castinfo_t;

castinfo_t	castorder[] = {
    {CC_ZOMBIE, MT_POSSESSED},
    {CC_SHOTGUN, MT_SHOTGUY},
    {CC_HEAVY, MT_CHAINGUY},
    {CC_IMP, MT_TROOP},
    {CC_DEMON, MT_SERGEANT},
    {CC_LOST, MT_SKULL},
    {CC_CACO, MT_HEAD},
    {CC_HELL, MT_KNIGHT},
    {CC_BARON, MT_BRUISER},
    {CC_ARACH, MT_BABY},
    {CC_PAIN, MT_PAIN},
    {CC_REVEN, MT_UNDEAD},
    {CC_MANCU, MT_FATSO},
    {CC_ARCH, MT_VILE},
    {CC_SPIDER, MT_SPIDER},
    {CC_CYBER, MT_CYBORG},
    {CC_HERO, MT_PLAYER},

    {NULL,0}
};

state_t*	caststate;


//
// F_StartCast
//
void F_StartCast (data_t* data)
{
    data->wipegamestate = -1;		// force a screen wipe
    data->castnum = 0;
    caststate = &states[mobjinfo[castorder[data->castnum].type].seestate];
    data->casttics = caststate->tics;
    data->castdeath = false;
    data->finalestage = F_STAGE_CAST;
    data->castframes = 0;
    data->castonmelee = 0;
    data->castattacking = false;
    S_ChangeMusic(data, mus_evil, true);
}


//
// F_CastTicker
//
void F_CastTicker (data_t* data)
{
    int		st;
    int		sfx;
	
    if (--data->casttics > 0)
	return;			// not time to change state yet
		
    if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
	// switch from deathstate to next monster
	data->castnum++;
	data->castdeath = false;
	if (castorder[data->castnum].name == NULL)
	    data->castnum = 0;
	if (mobjinfo[castorder[data->castnum].type].seesound)
	    S_StartSound(data, NULL, mobjinfo[castorder[data->castnum].type].seesound);
	caststate = &states[mobjinfo[castorder[data->castnum].type].seestate];
	data->castframes = 0;
    }
    else
    {
	// just advance to next state in animation
	if (caststate == &states[S_PLAY_ATK1])
	    goto stopattack;	// Oh, gross hack!
	st = caststate->nextstate;
	caststate = &states[st];
	data->castframes++;
	
	// sound hacks....
	switch (st)
	{
	  case S_PLAY_ATK1:	sfx = sfx_dshtgn; break;
	  case S_POSS_ATK2:	sfx = sfx_pistol; break;
	  case S_SPOS_ATK2:	sfx = sfx_shotgn; break;
	  case S_VILE_ATK2:	sfx = sfx_vilatk; break;
	  case S_SKEL_FIST2:	sfx = sfx_skeswg; break;
	  case S_SKEL_FIST4:	sfx = sfx_skepch; break;
	  case S_SKEL_MISS2:	sfx = sfx_skeatk; break;
	  case S_FATT_ATK8:
	  case S_FATT_ATK5:
	  case S_FATT_ATK2:	sfx = sfx_firsht; break;
	  case S_CPOS_ATK2:
	  case S_CPOS_ATK3:
	  case S_CPOS_ATK4:	sfx = sfx_shotgn; break;
	  case S_TROO_ATK3:	sfx = sfx_claw; break;
	  case S_SARG_ATK2:	sfx = sfx_sgtatk; break;
	  case S_BOSS_ATK2:
	  case S_BOS2_ATK2:
	  case S_HEAD_ATK2:	sfx = sfx_firsht; break;
	  case S_SKULL_ATK2:	sfx = sfx_sklatk; break;
	  case S_SPID_ATK2:
	  case S_SPID_ATK3:	sfx = sfx_shotgn; break;
	  case S_BSPI_ATK2:	sfx = sfx_plasma; break;
	  case S_CYBER_ATK2:
	  case S_CYBER_ATK4:
	  case S_CYBER_ATK6:	sfx = sfx_rlaunc; break;
	  case S_PAIN_ATK3:	sfx = sfx_sklatk; break;
	  default: sfx = 0; break;
	}
		
	if (sfx)
	    S_StartSound(data, NULL, sfx);
    }
	
    if (data->castframes == 12)
    {
	// go into attack frame
	data->castattacking = true;
	if (data->castonmelee)
	    caststate=&states[mobjinfo[castorder[data->castnum].type].meleestate];
	else
	    caststate=&states[mobjinfo[castorder[data->castnum].type].missilestate];
	data->castonmelee ^= 1;
	if (caststate == &states[S_NULL])
	{
	    if (data->castonmelee)
		caststate=
		    &states[mobjinfo[castorder[data->castnum].type].meleestate];
	    else
		caststate=
		    &states[mobjinfo[castorder[data->castnum].type].missilestate];
	}
    }
	
    if (data->castattacking)
    {
	if (data->castframes == 24
	    ||	caststate == &states[mobjinfo[castorder[data->castnum].type].seestate] )
	{
	  stopattack:
	    data->castattacking = false;
	    data->castframes = 0;
	    caststate = &states[mobjinfo[castorder[data->castnum].type].seestate];
	}
    }
	
    data->casttics = caststate->tics;
    if (data->casttics == -1)
	data->casttics = 15;
}


//
// F_CastResponder
//

boolean F_CastResponder (data_t* data, event_t* ev)
{
    if (ev->type != ev_keydown)
	return false;
		
    if (data->castdeath)
	return true;			// already in dying frames
		
    // go into death frame
    data->castdeath = true;
    caststate = &states[mobjinfo[castorder[data->castnum].type].deathstate];
    data->casttics = caststate->tics;
    data->castframes = 0;
    data->castattacking = false;
    if (mobjinfo[castorder[data->castnum].type].deathsound)
	S_StartSound(data, NULL, mobjinfo[castorder[data->castnum].type].deathsound);
	
    return true;
}


void F_CastPrint (data_t* data, char* text)
{
    char*	ch;
    int		c;
    int		cx;
    int		w;
    int		width;
    
    // find width
    ch = text;
    width = 0;
	
    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    width += 4;
	    continue;
	}
		
	w = SHORT (hu_font[c]->width);
	width += w;
    }
    
    // draw it
    cx = 160-width/2;
    ch = text;
    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}
		
	w = SHORT (hu_font[c]->width);
	V_DrawPatch(data, cx, 180, hu_font[c]);
	cx+=w;
    }
	
}


//
// F_CastDrawer
//

void F_CastDrawer (data_t* data)
{
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    boolean		flip;
    patch_t*		patch;
    
    // erase the entire screen to a background
    V_DrawPatch(data, 0, 0, W_CacheLumpName (DEH_String("BOSSBACK"), PU_CACHE));

    F_CastPrint (data, DEH_String(castorder[data->castnum].name));
    
    // draw the current frame in the middle of the screen
    sprdef = &sprites[caststate->sprite];
    sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];
			
    patch = W_CacheLumpNum (lump+data->firstspritelump, PU_CACHE);
    if (flip)
	V_DrawPatchFlipped(data, 160, 170, patch);
    else
	V_DrawPatch(data, 160, 170, patch);
}


//
// F_DrawPatchCol
//
void
F_DrawPatchCol
( data_t* data,
  int		x,
  patch_t*	patch,
  int		col )
{
    column_t*	column;
    byte*	source;
    byte*	dest;
    byte*	desttop;
    int		count;
	
    column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
    desttop = data->I_VideoBuffer + x;

    // step through the posts in a column
    while (column->topdelta != 0xff )
    {
	source = (byte *)column + 3;
	dest = desttop + column->topdelta*SCREENWIDTH;
	count = column->length;
		
	while (count--)
	{
	    *dest = *source++;
	    dest += SCREENWIDTH;
	}
	column = (column_t *)(  (byte *)column + column->length + 4 );
    }
}


//
// F_BunnyScroll
//
void F_BunnyScroll (data_t* data)
{
    signed int  scrolled;
    int		x;
    patch_t*	p1;
    patch_t*	p2;
    char	name[10];
    int		stage;
    static int	laststage;
		
    p1 = W_CacheLumpName (DEH_String("PFUB2"), PU_LEVEL);
    p2 = W_CacheLumpName (DEH_String("PFUB1"), PU_LEVEL);

    V_MarkRect(data, 0, 0, SCREENWIDTH, SCREENHEIGHT);
	
    scrolled = (320 - ((signed int) data->finalecount-230)/2);
    if (scrolled > 320)
	scrolled = 320;
    if (scrolled < 0)
	scrolled = 0;
		
    for ( x=0 ; x<SCREENWIDTH ; x++)
    {
	if (x+scrolled < 320)
	    F_DrawPatchCol (data, x, p1, x+scrolled);
	else
	    F_DrawPatchCol (data, x, p2, x+scrolled - 320);		
    }
	
    if (data->finalecount < 1130)
	return;
    if (data->finalecount < 1180)
    {
        V_DrawPatch(data, (SCREENWIDTH - 13 * 8) / 2,
                    (SCREENHEIGHT - 8 * 8) / 2, 
                    W_CacheLumpName(DEH_String("END0"), PU_CACHE));
	laststage = 0;
	return;
    }
	
    stage = (data->finalecount-1180) / 5;
    if (stage > 6)
	stage = 6;
    if (stage > laststage)
    {
	S_StartSound(data, NULL, sfx_pistol);
	laststage = stage;
    }
	
    DEH_snprintf(name, 10, "END%i", stage);
    V_DrawPatch(data, (SCREENWIDTH - 13 * 8) / 2, 
                (SCREENHEIGHT - 8 * 8) / 2, 
                W_CacheLumpName (name,PU_CACHE));
}

static void F_ArtScreenDrawer(data_t* data)
{
    char *lumpname;
    
    if (data->gameepisode == 3)
    {
        F_BunnyScroll(data);
    }
    else
    {
        switch (data->gameepisode)
        {
            case 1:
                if (data->gamemode == retail)
                {
                    lumpname = "CREDIT";
                }
                else
                {
                    lumpname = "HELP2";
                }
                break;
            case 2:
                lumpname = "VICTORY2";
                break;
            case 4:
                lumpname = "ENDPIC";
                break;
            default:
                return;
        }

        lumpname = DEH_String(lumpname);

        V_DrawPatch(data, 0, 0, W_CacheLumpName(lumpname, PU_CACHE));
    }
}

//
// F_Drawer
//
void F_Drawer (data_t* data)
{
    switch (data->finalestage)
    {
        case F_STAGE_CAST:
            F_CastDrawer(data);
            break;
        case F_STAGE_TEXT:
            F_TextWrite(data);
            break;
        case F_STAGE_ARTSCREEN:
            F_ArtScreenDrawer(data);
            break;
    }
}


