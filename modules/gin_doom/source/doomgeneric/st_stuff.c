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
//	Status bar code.
//	Does the face/direction indicator animatin.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//



#include <stdio.h>

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "w_wad.h"

#include "deh_main.h"
#include "deh_misc.h"
#include "doomdef.h"
#include "doomkeys.h"

#include "g_game.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

#include "p_local.h"
#include "p_inter.h"

#include "am_map.h"
#include "m_cheat.h"

#include "s_sound.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

//
// STATUS BAR DATA
//


// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS		1
#define STARTBONUSPALS		9
#define NUMREDPALS			8
#define NUMBONUSPALS		4
// Radiation suit, green shift.
#define RADIATIONPAL		13

// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY		96

// For Responder
#define ST_TOGGLECHAT		KEY_ENTER

// Location of status bar
#define ST_X				0
#define ST_X2				104

#define ST_FX  			143
#define ST_FY  			169

// Should be set to patch width
//  for tall numbers later on
#define ST_TALLNUMWIDTH		(tallnum[0]->width)

// Number of status faces.
#define ST_NUMPAINFACES		5
#define ST_NUMSTRAIGHTFACES	3
#define ST_NUMTURNFACES		2
#define ST_NUMSPECIALFACES		3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES		2

#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_TURNOFFSET		(ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET		(ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET		(ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET		(ST_EVILGRINOFFSET + 1)
#define ST_GODFACE			(ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE			(ST_GODFACE+1)

#define ST_FACESX			143
#define ST_FACESY			168

#define ST_EVILGRINCOUNT		(2*TICRATE)
#define ST_STRAIGHTFACECOUNT	(TICRATE/2)
#define ST_TURNCOUNT		(1*TICRATE)
#define ST_OUCHCOUNT		(1*TICRATE)
#define ST_RAMPAGEDELAY		(2*TICRATE)

#define ST_MUCHPAIN			20


// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH		3	
#define ST_AMMOX			44
#define ST_AMMOY			171

// HEALTH number pos.
#define ST_HEALTHWIDTH		3	
#define ST_HEALTHX			90
#define ST_HEALTHY			171

// Weapon pos.
#define ST_ARMSX			111
#define ST_ARMSY			172
#define ST_ARMSBGX			104
#define ST_ARMSBGY			168
#define ST_ARMSXSPACE		12
#define ST_ARMSYSPACE		10

// Frags pos.
#define ST_FRAGSX			138
#define ST_FRAGSY			171	
#define ST_FRAGSWIDTH		2

// ARMOR number pos.
#define ST_ARMORWIDTH		3
#define ST_ARMORX			221
#define ST_ARMORY			171

// Key icon positions.
#define ST_KEY0WIDTH		8
#define ST_KEY0HEIGHT		5
#define ST_KEY0X			239
#define ST_KEY0Y			171
#define ST_KEY1WIDTH		ST_KEY0WIDTH
#define ST_KEY1X			239
#define ST_KEY1Y			181
#define ST_KEY2WIDTH		ST_KEY0WIDTH
#define ST_KEY2X			239
#define ST_KEY2Y			191

// Ammunition counter.
#define ST_AMMO0WIDTH		3
#define ST_AMMO0HEIGHT		6
#define ST_AMMO0X			288
#define ST_AMMO0Y			173
#define ST_AMMO1WIDTH		ST_AMMO0WIDTH
#define ST_AMMO1X			288
#define ST_AMMO1Y			179
#define ST_AMMO2WIDTH		ST_AMMO0WIDTH
#define ST_AMMO2X			288
#define ST_AMMO2Y			191
#define ST_AMMO3WIDTH		ST_AMMO0WIDTH
#define ST_AMMO3X			288
#define ST_AMMO3Y			185

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH		3
#define ST_MAXAMMO0HEIGHT		5
#define ST_MAXAMMO0X		314
#define ST_MAXAMMO0Y		173
#define ST_MAXAMMO1WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X		314
#define ST_MAXAMMO1Y		179
#define ST_MAXAMMO2WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X		314
#define ST_MAXAMMO2Y		191
#define ST_MAXAMMO3WIDTH		ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X		314
#define ST_MAXAMMO3Y		185

// pistol
#define ST_WEAPON0X			110 
#define ST_WEAPON0Y			172

// shotgun
#define ST_WEAPON1X			122 
#define ST_WEAPON1Y			172

// chain gun
#define ST_WEAPON2X			134 
#define ST_WEAPON2Y			172

// missile launcher
#define ST_WEAPON3X			110 
#define ST_WEAPON3Y			181

// plasma gun
#define ST_WEAPON4X			122 
#define ST_WEAPON4Y			181

 // bfg
#define ST_WEAPON5X			134
#define ST_WEAPON5Y			181

// WPNS title
#define ST_WPNSX			109 
#define ST_WPNSY			191

 // DETH title
#define ST_DETHX			109
#define ST_DETHY			191

//Incoming messages window location
//UNUSED
// #define ST_MSGTEXTX	   (data->viewwindowx)
// #define ST_MSGTEXTY	   (data->viewwindowy+data->viewheight-18)
#define ST_MSGTEXTX			0
#define ST_MSGTEXTY			0
// Dimensions given in characters.
#define ST_MSGWIDTH			52
// Or shall I say, in data->lines?
#define ST_MSGHEIGHT		1

#define ST_OUTTEXTX			0
#define ST_OUTTEXTY			6

// Width, in characters again.
#define ST_OUTWIDTH			52 
 // Height, in data->lines. 
#define ST_OUTHEIGHT		1

#define ST_MAPTITLEX \
    (SCREENWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH)

#define ST_MAPTITLEY		0
#define ST_MAPHEIGHT		1

// graphics are drawn to a backing screen and blitted to the real screen
byte                   *st_backing_screen;
	    
// main player in game

// ST_Start() has just been called

// lump number for PLAYPAL

// used for timing

// used for making messages go away

// used when in chat 

// whether in automap or first-person

// whether left-side main status bar is active

// whether status bar chat is active

// value of data->sb_st_chat before message popped up

// whether chat window has the cursor on

// !data->deathmatch

// !data->deathmatch && data->sb_st_statusbaron

// !data->deathmatch

// main bar left
static patch_t*		sbar;

// 0-9, tall numbers
static patch_t*		tallnum[10];

// tall % sign
static patch_t*		tallpercent;

// 0-9, short, yellow (,different!) numbers
static patch_t*		shortnum[10];

// 3 key-cards, 3 skulls
static patch_t*		keys[NUMCARDS]; 

// face status patches
static patch_t*		faces[ST_NUMFACES];

// face background
static patch_t*		faceback;

 // main bar right
static patch_t*		armsbg;

// weapon ownership patches
static patch_t*		arms[6][2]; 

// ready-weapon widget

 // in data->deathmatch only, summary of frags stats

// health widget

// arms background


// weapon ownership widgets

// face status widget

// keycard widgets

// armor widget

// ammo widgets

// max ammo widgets



 // number of frags so far in data->deathmatch

// used to use appopriately pained face

// used for evil grin

 // count until face changes

// current face index, used by data->sb_w_faces

// holds key-type for each key box on bar

// a random number per tick

cheatseq_t cheat_mus = CHEAT("idmus", 2);
cheatseq_t cheat_god = CHEAT("iddqd", 0);
cheatseq_t cheat_ammo = CHEAT("idkfa", 0);
cheatseq_t cheat_ammonokey = CHEAT("idfa", 0);
cheatseq_t cheat_noclip = CHEAT("idspispopd", 0);
cheatseq_t cheat_commercial_noclip = CHEAT("idclip", 0);

cheatseq_t	cheat_powerup[7] =
{
    CHEAT("idbeholdv", 0),
    CHEAT("idbeholds", 0),
    CHEAT("idbeholdi", 0),
    CHEAT("idbeholdr", 0),
    CHEAT("idbeholda", 0),
    CHEAT("idbeholdl", 0),
    CHEAT("idbehold", 0),
};

cheatseq_t cheat_choppers = CHEAT("idchoppers", 0);
cheatseq_t cheat_clev = CHEAT("idclev", 2);
cheatseq_t cheat_mypos = CHEAT("idmypos", 0);


//
// STATUS BAR CODE
//
void ST_Stop(data_t* data);

void ST_refreshBackground(data_t* data)
{

    if (data->sb_st_statusbaron)
    {
        V_UseBuffer(data, st_backing_screen);

	V_DrawPatch(data, ST_X, 0, sbar);

	if (data->netgame)
	    V_DrawPatch(data, ST_FX, 0, faceback);

        V_RestoreBuffer(data);

	V_CopyRect(data, ST_X, 0, st_backing_screen, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y);
    }

}


// Respond to keyboard input events,
//  intercept cheats.
boolean
ST_Responder (data_t* data, event_t* ev)
{
  int		i;
    
  // Filter automap on/off.
  if (ev->type == ev_keyup
      && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
  {
    switch(ev->data1)
    {
      case AM_MSGENTERED:
	data->sb_st_gamestate = AutomapState;
	data->sb_st_firsttime = true;
	break;
	
      case AM_MSGEXITED:
	//	fprintf(stderr, "AM exited\n");
	data->sb_st_gamestate = FirstPersonState;
	break;
    }
  }

  // if a user keypress...
  else if (ev->type == ev_keydown)
  {
    if (!data->netgame && data->gameskill != sk_nightmare)
    {
      // 'dqd' cheat for toggleable god mode
      if (cht_CheckCheat(&cheat_god, ev->data2))
      {
	data->sb_plyr->cheats ^= CF_GODMODE;
	if (data->sb_plyr->cheats & CF_GODMODE)
	{
	  if (data->sb_plyr->mo)
	    data->sb_plyr->mo->health = 100;
	  
	  data->sb_plyr->health = deh_god_mode_health;
	  data->sb_plyr->message = DEH_String(STSTR_DQDON);
	}
	else 
	  data->sb_plyr->message = DEH_String(STSTR_DQDOFF);
      }
      // 'fa' cheat for killer fucking arsenal
      else if (cht_CheckCheat(&cheat_ammonokey, ev->data2))
      {
	data->sb_plyr->armorpoints = deh_idfa_armor;
	data->sb_plyr->armortype = deh_idfa_armor_class;
	
	for (i=0;i<NUMWEAPONS;i++)
	  data->sb_plyr->weaponowned[i] = true;
	
	for (i=0;i<NUMAMMO;i++)
	  data->sb_plyr->ammo[i] = data->sb_plyr->maxammo[i];
	
	data->sb_plyr->message = DEH_String(STSTR_FAADDED);
      }
      // 'kfa' cheat for key full ammo
      else if (cht_CheckCheat(&cheat_ammo, ev->data2))
      {
	data->sb_plyr->armorpoints = deh_idkfa_armor;
	data->sb_plyr->armortype = deh_idkfa_armor_class;
	
	for (i=0;i<NUMWEAPONS;i++)
	  data->sb_plyr->weaponowned[i] = true;
	
	for (i=0;i<NUMAMMO;i++)
	  data->sb_plyr->ammo[i] = data->sb_plyr->maxammo[i];
	
	for (i=0;i<NUMCARDS;i++)
	  data->sb_plyr->cards[i] = true;
	
	data->sb_plyr->message = DEH_String(STSTR_KFAADDED);
      }
      // 'mus' cheat for changing music
      else if (cht_CheckCheat(&cheat_mus, ev->data2))
      {
	
	char	buf[3];
	int		musnum;
	
	data->sb_plyr->message = DEH_String(STSTR_MUS);
	cht_GetParam(&cheat_mus, buf);

        // Note: The original v1.9 had a bug that tried to play back
        // the Doom II music regardless of data->gamemode.  This was fixed
        // in the Ultimate Doom executable so that it would work for
        // the Doom 1 music as well.

	if (data->gamemode == commercial || data->gameversion < exe_ultimate)
	{
	  musnum = mus_runnin + (buf[0]-'0')*10 + buf[1]-'0' - 1;
	  
	  if (((buf[0]-'0')*10 + buf[1]-'0') > 35)
	    data->sb_plyr->message = DEH_String(STSTR_NOMUS);
	  else
	    S_ChangeMusic(data, musnum, 1);
	}
	else
	{
	  musnum = mus_e1m1 + (buf[0]-'1')*9 + (buf[1]-'1');
	  
	  if (((buf[0]-'1')*9 + buf[1]-'1') > 31)
	    data->sb_plyr->message = DEH_String(STSTR_NOMUS);
	  else
	    S_ChangeMusic(data, musnum, 1);
	}
      }
      else if ( (logical_gamemission == doom 
                 && cht_CheckCheat(&cheat_noclip, ev->data2))
             || (logical_gamemission != doom 
                 && cht_CheckCheat(&cheat_commercial_noclip,ev->data2)))
      {	
        // Noclip cheat.
        // For Doom 1, use the idspipsopd cheat; for all others, use
        // idclip

	data->sb_plyr->cheats ^= CF_NOCLIP;
	
	if (data->sb_plyr->cheats & CF_NOCLIP)
	  data->sb_plyr->message = DEH_String(STSTR_NCON);
	else
	  data->sb_plyr->message = DEH_String(STSTR_NCOFF);
      }
      // 'behold?' power-up cheats
      for (i=0;i<6;i++)
      {
	if (cht_CheckCheat(&cheat_powerup[i], ev->data2))
	{
	  if (!data->sb_plyr->powers[i])
	    P_GivePower( data->sb_plyr, i);
	  else if (i!=pw_strength)
	    data->sb_plyr->powers[i] = 1;
	  else
	    data->sb_plyr->powers[i] = 0;
	  
	  data->sb_plyr->message = DEH_String(STSTR_BEHOLDX);
	}
      }
      
      // 'behold' power-up menu
      if (cht_CheckCheat(&cheat_powerup[6], ev->data2))
      {
	data->sb_plyr->message = DEH_String(STSTR_BEHOLD);
      }
      // 'choppers' invulnerability & chainsaw
      else if (cht_CheckCheat(&cheat_choppers, ev->data2))
      {
	data->sb_plyr->weaponowned[wp_chainsaw] = true;
	data->sb_plyr->powers[pw_invulnerability] = true;
	data->sb_plyr->message = DEH_String(STSTR_CHOPPERS);
      }
      // 'mypos' for player position
      else if (cht_CheckCheat(&cheat_mypos, ev->data2))
      {
        static char buf[ST_MSGWIDTH];
        M_snprintf(buf, sizeof(buf), "ang=0x%x;x,y=(0x%x,0x%x)",
                   data->players[data->consoleplayer].mo->angle,
                   data->players[data->consoleplayer].mo->x,
                   data->players[data->consoleplayer].mo->y);
        data->sb_plyr->message = buf;
      }
    }
    
    // 'clev' change-level cheat
    if (!data->netgame && cht_CheckCheat(&cheat_clev, ev->data2))
    {
      char		buf[3];
      int		epsd;
      int		map;
      
      cht_GetParam(&cheat_clev, buf);
      
      if (data->gamemode == commercial)
      {
	epsd = 1;
	map = (buf[0] - '0')*10 + buf[1] - '0';
      }
      else
      {
	epsd = buf[0] - '0';
	map = buf[1] - '0';
      }

      // Chex.exe always warps to episode 1.

      if (data->gameversion == exe_chex)
      {
        epsd = 1;
      }

      // Catch invalid maps.
      if (epsd < 1)
	return false;

      if (map < 1)
	return false;

      // Ohmygod - this is not going to work.
      if ((data->gamemode == retail)
	  && ((epsd > 4) || (map > 9)))
	return false;

      if ((data->gamemode == registered)
	  && ((epsd > 3) || (map > 9)))
	return false;

      if ((data->gamemode == shareware)
	  && ((epsd > 1) || (map > 9)))
	return false;

      // The source release has this check as map > 34. However, Vanilla
      // Doom allows IDCLEV up to MAP40 even though it normally crashes.
      if ((data->gamemode == commercial)
	&& (( epsd > 1) || (map > 40)))
	return false;

      // So be it.
      data->sb_plyr->message = DEH_String(STSTR_CLEV);
      G_DeferedInitNew(data, data->gameskill, epsd, map);
    }
  }
  return false;
}



int ST_calcPainOffset(data_t* data)
{
    int		health;
    static int	lastcalc;
    static int	oldhealth = -1;
    
    health = data->sb_plyr->health > 100 ? 100 : data->sb_plyr->health;

    if (health != oldhealth)
    {
	lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
	oldhealth = health;
    }
    return lastcalc;
}


//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void ST_updateFaceWidget(data_t* data)
{
    int		i;
    angle_t	badguyangle;
    angle_t	diffang;
    static int	lastattackdown = -1;
    static int	priority = 0;
    boolean	doevilgrin;

    if (priority < 10)
    {
	// dead
	if (!data->sb_plyr->health)
	{
	    priority = 9;
	    data->sb_st_faceindex = ST_DEADFACE;
	    data->sb_st_facecount = 1;
	}
    }

    if (priority < 9)
    {
	if (data->sb_plyr->bonuscount)
	{
	    // picking up bonus
	    doevilgrin = false;

	    for (i=0;i<NUMWEAPONS;i++)
	    {
		if (data->sb_oldweaponsowned[i] != data->sb_plyr->weaponowned[i])
		{
		    doevilgrin = true;
		    data->sb_oldweaponsowned[i] = data->sb_plyr->weaponowned[i];
		}
	    }
	    if (doevilgrin) 
	    {
		// evil grin if just picked up weapon
		priority = 8;
		data->sb_st_facecount = ST_EVILGRINCOUNT;
		data->sb_st_faceindex = ST_calcPainOffset(data) + ST_EVILGRINOFFSET;
	    }
	}

    }
  
    if (priority < 8)
    {
	if (data->sb_plyr->damagecount
	    && data->sb_plyr->attacker
	    && data->sb_plyr->attacker != data->sb_plyr->mo)
	{
	    // being attacked
	    priority = 7;
	    
	    if (data->sb_plyr->health - data->sb_st_oldhealth > ST_MUCHPAIN)
	    {
		data->sb_st_facecount = ST_TURNCOUNT;
		data->sb_st_faceindex = ST_calcPainOffset(data) + ST_OUCHOFFSET;
	    }
	    else
	    {
		badguyangle = R_PointToAngle2(data, data->sb_plyr->mo->x,
					      data->sb_plyr->mo->y,
					      data->sb_plyr->attacker->x,
					      data->sb_plyr->attacker->y);
		
		if (badguyangle > data->sb_plyr->mo->angle)
		{
		    // whether right or left
		    diffang = badguyangle - data->sb_plyr->mo->angle;
		    i = diffang > ANG180; 
		}
		else
		{
		    // whether left or right
		    diffang = data->sb_plyr->mo->angle - badguyangle;
		    i = diffang <= ANG180; 
		} // confusing, aint it?

		
		data->sb_st_facecount = ST_TURNCOUNT;
		data->sb_st_faceindex = ST_calcPainOffset(data);
		
		if (diffang < ANG45)
		{
		    // head-on    
		    data->sb_st_faceindex += ST_RAMPAGEOFFSET;
		}
		else if (i)
		{
		    // turn face right
		    data->sb_st_faceindex += ST_TURNOFFSET;
		}
		else
		{
		    // turn face left
		    data->sb_st_faceindex += ST_TURNOFFSET+1;
		}
	    }
	}
    }
  
    if (priority < 7)
    {
	// getting hurt because of your own damn stupidity
	if (data->sb_plyr->damagecount)
	{
	    if (data->sb_plyr->health - data->sb_st_oldhealth > ST_MUCHPAIN)
	    {
		priority = 7;
		data->sb_st_facecount = ST_TURNCOUNT;
		data->sb_st_faceindex = ST_calcPainOffset(data) + ST_OUCHOFFSET;
	    }
	    else
	    {
		priority = 6;
		data->sb_st_facecount = ST_TURNCOUNT;
		data->sb_st_faceindex = ST_calcPainOffset(data) + ST_RAMPAGEOFFSET;
	    }

	}

    }
  
    if (priority < 6)
    {
	// rapid firing
	if (data->sb_plyr->attackdown)
	{
	    if (lastattackdown==-1)
		lastattackdown = ST_RAMPAGEDELAY;
	    else if (!--lastattackdown)
	    {
		priority = 5;
		data->sb_st_faceindex = ST_calcPainOffset(data) + ST_RAMPAGEOFFSET;
		data->sb_st_facecount = 1;
		lastattackdown = 1;
	    }
	}
	else
	    lastattackdown = -1;

    }
  
    if (priority < 5)
    {
	// invulnerability
	if ((data->sb_plyr->cheats & CF_GODMODE)
	    || data->sb_plyr->powers[pw_invulnerability])
	{
	    priority = 4;

	    data->sb_st_faceindex = ST_GODFACE;
	    data->sb_st_facecount = 1;

	}

    }

    // look left or look right if the facecount has timed out
    if (!data->sb_st_facecount)
    {
	data->sb_st_faceindex = ST_calcPainOffset(data) + (data->sb_st_randomnumber % 3);
	data->sb_st_facecount = ST_STRAIGHTFACECOUNT;
	priority = 0;
    }

    data->sb_st_facecount--;

}

void ST_updateWidgets(data_t* data)
{
    static int	largeammo = 1994; // means "n/a"
    int		i;

    // must redirect the pointer if the ready weapon has changed.
    //  if (data->sb_w_ready.data != data->sb_plyr->readyweapon)
    //  {
    if (weaponinfo[data->sb_plyr->readyweapon].ammo == am_noammo)
	data->sb_w_ready.num = &largeammo;
    else
	data->sb_w_ready.num = &data->sb_plyr->ammo[weaponinfo[data->sb_plyr->readyweapon].ammo];
    //{
    // static int tic=0;
    // static int dir=-1;
    // if (!(tic&15))
    //   data->sb_plyr->ammo[weaponinfo[data->sb_plyr->readyweapon].ammo]+=dir;
    // if (data->sb_plyr->ammo[weaponinfo[data->sb_plyr->readyweapon].ammo] == -100)
    //   dir = 1;
    // tic++;
    // }
    data->sb_w_ready.data = data->sb_plyr->readyweapon;

    // if (*data->sb_w_ready.on)
    //  STlib_updateNum(data, &data->sb_w_ready, true);
    // refresh weapon change
    //  }

    // update keycard multiple widgets
    for (i=0;i<3;i++)
    {
	data->sb_keyboxes[i] = data->sb_plyr->cards[i] ? i : -1;

	if (data->sb_plyr->cards[i+3])
	    data->sb_keyboxes[i] = i+3;
    }

    // refresh everything if this is him coming back to life
    ST_updateFaceWidget(data);

    // used by the data->sb_w_armsbg widget
    data->sb_st_notdeathmatch = !data->deathmatch;
    
    // used by data->sb_w_arms[] widgets
    data->sb_st_armson = data->sb_st_statusbaron && !data->deathmatch; 

    // used by data->sb_w_frags widget
    data->sb_st_fragson = data->deathmatch && data->sb_st_statusbaron; 
    data->sb_st_fragscount = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (i != data->consoleplayer)
	    data->sb_st_fragscount += data->sb_plyr->frags[i];
	else
	    data->sb_st_fragscount -= data->sb_plyr->frags[i];
    }

    // get rid of chat window if up because of message
    if (!--data->sb_st_msgcounter)
	data->sb_st_chat = data->sb_st_oldchat;

}

void ST_Ticker (data_t* data)
{

    data->sb_st_clock++;
    data->sb_st_randomnumber = M_Random (data);
    ST_updateWidgets(data);
    data->sb_st_oldhealth = data->sb_plyr->health;

}


void ST_doPaletteStuff(data_t* data)
{

    int		palette;
    byte*	pal;
    int		cnt;
    int		bzc;

    cnt = data->sb_plyr->damagecount;

    if (data->sb_plyr->powers[pw_strength])
    {
	// slowly fade the berzerk out
  	bzc = 12 - (data->sb_plyr->powers[pw_strength]>>6);

	if (bzc > cnt)
	    cnt = bzc;
    }
	
    if (cnt)
    {
	palette = (cnt+7)>>3;
	
	if (palette >= NUMREDPALS)
	    palette = NUMREDPALS-1;

	palette += STARTREDPALS;
    }

    else if (data->sb_plyr->bonuscount)
    {
	palette = (data->sb_plyr->bonuscount+7)>>3;

	if (palette >= NUMBONUSPALS)
	    palette = NUMBONUSPALS-1;

	palette += STARTBONUSPALS;
    }

    else if ( data->sb_plyr->powers[pw_ironfeet] > 4*32
	      || data->sb_plyr->powers[pw_ironfeet]&8)
	palette = RADIATIONPAL;
    else
	palette = 0;

    // In Chex Quest, the player never sees red.  Instead, the
    // radiation suit palette is used to tint the screen green,
    // as though the player is being covered in goo by an
    // attacking flemoid.

    if (data->gameversion == exe_chex
     && palette >= STARTREDPALS && palette < STARTREDPALS + NUMREDPALS)
    {
        palette = RADIATIONPAL;
    }

    if (palette != data->sb_st_palette)
    {
	data->sb_st_palette = palette;
	pal = (byte *) W_CacheLumpNum (data->sb_lu_palette, PU_CACHE)+palette*768;
	I_SetPalette (data, pal);
    }

}

void ST_drawWidgets(data_t* data, boolean refresh)
{
    int		i;

    // used by data->sb_w_arms[] widgets
    data->sb_st_armson = data->sb_st_statusbaron && !data->deathmatch;

    // used by data->sb_w_frags widget
    data->sb_st_fragson = data->deathmatch && data->sb_st_statusbaron; 

    STlib_updateNum(data, &data->sb_w_ready, refresh);

    for (i=0;i<4;i++)
    {
	STlib_updateNum(data, &data->sb_w_ammo[i], refresh);
	STlib_updateNum(data, &data->sb_w_maxammo[i], refresh);
    }

    STlib_updatePercent(data, &data->sb_w_health, refresh);
    STlib_updatePercent(data, &data->sb_w_armor, refresh);

    STlib_updateBinIcon(data, &data->sb_w_armsbg, refresh);

    for (i=0;i<6;i++)
	STlib_updateMultIcon(data, &data->sb_w_arms[i], refresh);

    STlib_updateMultIcon(data, &data->sb_w_faces, refresh);

    for (i=0;i<3;i++)
	STlib_updateMultIcon(data, &data->sb_w_keyboxes[i], refresh);

    STlib_updateNum(data, &data->sb_w_frags, refresh);

}

void ST_doRefresh(data_t* data)
{

    data->sb_st_firsttime = false;

    // draw status bar background to off-screen buff
    ST_refreshBackground(data);

    // and refresh all widgets
    ST_drawWidgets(data, true);

}

void ST_diffDraw(data_t* data)
{
    // update all widgets
    ST_drawWidgets(data, false);
}

void ST_Drawer (data_t* data, boolean fullscreen, boolean refresh)
{
  
    data->sb_st_statusbaron = (!fullscreen) || data->automapactive;
    data->sb_st_firsttime = data->sb_st_firsttime || refresh;

    // Do red-/gold-shifts from damage/items
    ST_doPaletteStuff(data);

    // If just after ST_Start(), refresh all
    if (data->sb_st_firsttime) ST_doRefresh(data);
    // Otherwise, update as little as possible
    else ST_diffDraw(data);

}

typedef void (*load_callback_t)(char *lumpname, patch_t **variable); 

// Iterates through all graphics to be loaded or unloaded, along with
// the variable they use, invoking the specified callback function.

static void ST_loadUnloadGraphics(data_t* data, load_callback_t callback)
{

    int		i;
    int		j;
    int		facenum;
    
    char	namebuf[9];

    // Load the numbers, tall and short
    for (i=0;i<10;i++)
    {
	DEH_snprintf(namebuf, 9, "STTNUM%d", i);
        callback(namebuf, &tallnum[i]);

	DEH_snprintf(namebuf, 9, "STYSNUM%d", i);
        callback(namebuf, &shortnum[i]);
    }

    // Load percent key.
    //Note: why not load STMINUS here, too?

    callback(DEH_String("STTPRCNT"), &tallpercent);

    // key cards
    for (i=0;i<NUMCARDS;i++)
    {
    	DEH_snprintf(namebuf, 9, "STKEYS%d", i);
        callback(namebuf, &keys[i]);
    }

    // arms background
    callback(DEH_String("STARMS"), &armsbg);

    // arms ownership widgets
    for (i=0; i<6; i++)
    {
    	DEH_snprintf(namebuf, 9, "STGNUM%d", i+2);

    	// gray #
        callback(namebuf, &arms[i][0]);

        // yellow #
        arms[i][1] = shortnum[i+2];
    }

    // face backgrounds for different color data->players
    DEH_snprintf(namebuf, 9, "STFB%d", data->consoleplayer);
    callback(namebuf, &faceback);

    // status bar background bits
    callback(DEH_String("STBAR"), &sbar);

    // face states
    facenum = 0;
    for (i=0; i<ST_NUMPAINFACES; i++)
    {
	for (j=0; j<ST_NUMSTRAIGHTFACES; j++)
	{
	    DEH_snprintf(namebuf, 9, "STFST%d%d", i, j);
            callback(namebuf, &faces[facenum]);
            ++facenum;
	}
	DEH_snprintf(namebuf, 9, "STFTR%d0", i);	// turn right
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFTL%d0", i);	// turn left
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFOUCH%d", i);	// ouch!
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFEVL%d", i);	// evil grin ;)
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFKILL%d", i);	// pissed off
        callback(namebuf, &faces[facenum]);
        ++facenum;
    }

    callback(DEH_String("STFGOD0"), &faces[facenum]);
    ++facenum;
    callback(DEH_String("STFDEAD0"), &faces[facenum]);
    ++facenum;
}

static void ST_loadCallback(char *lumpname, patch_t **variable)
{
    *variable = W_CacheLumpName(lumpname, PU_STATIC);
}

void ST_loadGraphics(data_t* data)
{
    ST_loadUnloadGraphics(data, ST_loadCallback);
}

void ST_loadData(data_t* data)
{
    data->sb_lu_palette = W_GetNumForName (DEH_String("PLAYPAL"));
    ST_loadGraphics(data);
}

static void ST_unloadCallback(char *lumpname, patch_t **variable)
{
    W_ReleaseLumpName(lumpname);
    *variable = NULL;
}

void ST_unloadGraphics(data_t* data)
{
    ST_loadUnloadGraphics(data, ST_unloadCallback);
}

void ST_unloadData(data_t* data)
{
    ST_unloadGraphics(data);
}

void ST_initData(data_t* data)
{

    int		i;

    data->sb_st_firsttime = true;
    data->sb_plyr = &data->players[data->consoleplayer];

    data->sb_st_clock = 0;
    data->sb_st_chatstate = StartChatState;
    data->sb_st_gamestate = FirstPersonState;

    data->sb_st_statusbaron = true;
    data->sb_st_oldchat = data->sb_st_chat = false;
    data->sb_st_cursoron = false;

    data->sb_st_faceindex = 0;
    data->sb_st_palette = -1;

    data->sb_st_oldhealth = -1;

    for (i=0;i<NUMWEAPONS;i++)
	data->sb_oldweaponsowned[i] = data->sb_plyr->weaponowned[i];

    for (i=0;i<3;i++)
	data->sb_keyboxes[i] = -1;

    STlib_init(data);

}



void ST_createWidgets(data_t* data)
{

    int i;

    // ready weapon ammo
    STlib_initNum(data, &data->sb_w_ready,
		  ST_AMMOX,
		  ST_AMMOY,
		  tallnum,
		  &data->sb_plyr->ammo[weaponinfo[data->sb_plyr->readyweapon].ammo],
		  &data->sb_st_statusbaron,
		  ST_AMMOWIDTH );

    // the last weapon type
    data->sb_w_ready.data = data->sb_plyr->readyweapon; 

    // health percentage
    STlib_initPercent(data, &data->sb_w_health,
		      ST_HEALTHX,
		      ST_HEALTHY,
		      tallnum,
		      &data->sb_plyr->health,
		      &data->sb_st_statusbaron,
		      tallpercent);

    // arms background
    STlib_initBinIcon(data, &data->sb_w_armsbg,
		      ST_ARMSBGX,
		      ST_ARMSBGY,
		      armsbg,
		      &data->sb_st_notdeathmatch,
		      &data->sb_st_statusbaron);

    // weapons owned
    for(i=0;i<6;i++)
    {
	STlib_initMultIcon(data, &data->sb_w_arms[i],
			   ST_ARMSX+(i%3)*ST_ARMSXSPACE,
			   ST_ARMSY+(i/3)*ST_ARMSYSPACE,
			   arms[i], (int *) &data->sb_plyr->weaponowned[i+1],
			   &data->sb_st_armson);
    }

    // frags sum
    STlib_initNum(data, &data->sb_w_frags,
		  ST_FRAGSX,
		  ST_FRAGSY,
		  tallnum,
		  &data->sb_st_fragscount,
		  &data->sb_st_fragson,
		  ST_FRAGSWIDTH);

    // faces
    STlib_initMultIcon(data, &data->sb_w_faces,
		       ST_FACESX,
		       ST_FACESY,
		       faces,
		       &data->sb_st_faceindex,
		       &data->sb_st_statusbaron);

    // armor percentage - should be colored later
    STlib_initPercent(data, &data->sb_w_armor,
		      ST_ARMORX,
		      ST_ARMORY,
		      tallnum,
		      &data->sb_plyr->armorpoints,
		      &data->sb_st_statusbaron, tallpercent);

    // data->sb_keyboxes 0-2
    STlib_initMultIcon(data, &data->sb_w_keyboxes[0],
		       ST_KEY0X,
		       ST_KEY0Y,
		       keys,
		       &data->sb_keyboxes[0],
		       &data->sb_st_statusbaron);
    
    STlib_initMultIcon(data, &data->sb_w_keyboxes[1],
		       ST_KEY1X,
		       ST_KEY1Y,
		       keys,
		       &data->sb_keyboxes[1],
		       &data->sb_st_statusbaron);

    STlib_initMultIcon(data, &data->sb_w_keyboxes[2],
		       ST_KEY2X,
		       ST_KEY2Y,
		       keys,
		       &data->sb_keyboxes[2],
		       &data->sb_st_statusbaron);

    // ammo count (all four kinds)
    STlib_initNum(data, &data->sb_w_ammo[0],
		  ST_AMMO0X,
		  ST_AMMO0Y,
		  shortnum,
		  &data->sb_plyr->ammo[0],
		  &data->sb_st_statusbaron,
		  ST_AMMO0WIDTH);

    STlib_initNum(data, &data->sb_w_ammo[1],
		  ST_AMMO1X,
		  ST_AMMO1Y,
		  shortnum,
		  &data->sb_plyr->ammo[1],
		  &data->sb_st_statusbaron,
		  ST_AMMO1WIDTH);

    STlib_initNum(data, &data->sb_w_ammo[2],
		  ST_AMMO2X,
		  ST_AMMO2Y,
		  shortnum,
		  &data->sb_plyr->ammo[2],
		  &data->sb_st_statusbaron,
		  ST_AMMO2WIDTH);
    
    STlib_initNum(data, &data->sb_w_ammo[3],
		  ST_AMMO3X,
		  ST_AMMO3Y,
		  shortnum,
		  &data->sb_plyr->ammo[3],
		  &data->sb_st_statusbaron,
		  ST_AMMO3WIDTH);

    // max ammo count (all four kinds)
    STlib_initNum(data, &data->sb_w_maxammo[0],
		  ST_MAXAMMO0X,
		  ST_MAXAMMO0Y,
		  shortnum,
		  &data->sb_plyr->maxammo[0],
		  &data->sb_st_statusbaron,
		  ST_MAXAMMO0WIDTH);

    STlib_initNum(data, &data->sb_w_maxammo[1],
		  ST_MAXAMMO1X,
		  ST_MAXAMMO1Y,
		  shortnum,
		  &data->sb_plyr->maxammo[1],
		  &data->sb_st_statusbaron,
		  ST_MAXAMMO1WIDTH);

    STlib_initNum(data, &data->sb_w_maxammo[2],
		  ST_MAXAMMO2X,
		  ST_MAXAMMO2Y,
		  shortnum,
		  &data->sb_plyr->maxammo[2],
		  &data->sb_st_statusbaron,
		  ST_MAXAMMO2WIDTH);
    
    STlib_initNum(data, &data->sb_w_maxammo[3],
		  ST_MAXAMMO3X,
		  ST_MAXAMMO3Y,
		  shortnum,
		  &data->sb_plyr->maxammo[3],
		  &data->sb_st_statusbaron,
		  ST_MAXAMMO3WIDTH);

}



void ST_Start(data_t* data)
{

    if (!data->sb_st_stopped)
	ST_Stop(data);

    ST_initData(data);
    ST_createWidgets(data);
    data->sb_st_stopped = false;

}

void ST_Stop(data_t* data)
{
    if (data->sb_st_stopped)
	return;

    I_SetPalette (data, W_CacheLumpNum (data->sb_lu_palette, PU_CACHE));

    data->sb_st_stopped = true;
}

void ST_Init(data_t* data)
{
    ST_loadData(data);
    st_backing_screen = (byte *) Z_Malloc(ST_WIDTH * ST_HEIGHT, PU_STATIC, 0);
}

