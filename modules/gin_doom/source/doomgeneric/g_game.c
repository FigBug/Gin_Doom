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
// DESCRIPTION:  none
//



#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "doomdef.h" 
#include "doomkeys.h"
#include "doomstat.h"

#include "deh_main.h"
#include "deh_misc.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "statdump.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h" 

#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"



#include "g_game.h"


#define SAVEGAMESIZE	0x2c000

void	G_ReadDemoTiccmd (data_t* data, ticcmd_t* cmd);
void	G_WriteDemoTiccmd (data_t* data, ticcmd_t* cmd);
void	G_PlayerReborn (data_t* data, int player);
 
void	G_DoReborn (data_t* data, int playernum);
 
void	G_DoLoadLevel (data_t* data);
void	G_DoNewGame (data_t* data);
void	G_DoPlayDemo (data_t* data);
void	G_DoCompleted (data_t* data);
void	G_DoVictory (data_t* data);
void	G_DoWorldDone (data_t* data);
void	G_DoSaveGame (data_t* data);
 
// Gamestate the last time G_Ticker was called.
gamestate_t     oldgamestate;



// If non-zero, exit the level after this number of minutes.


 
 
 

 
 
char           *demoname;
byte*		demobuffer;
byte*		demo_p;
byte*		demoend; 
 

boolean         testcontrols = false;    // Invoked by setup to test controls
int             testcontrols_mousespeed;
 

 
 
 
#define MAXPLMOVE		(forwardmove[1]) 
 
#define TURBOTHRESHOLD	0x32

fixed_t         forwardmove[2] = {0x19, 0x32}; 
fixed_t         sidemove[2] = {0x18, 0x28}; 
fixed_t         angleturn[3] = {640, 1280, 320};    // + slow turn 

static int *weapon_keys[] = {
    &key_weapon1,
    &key_weapon2,
    &key_weapon3,
    &key_weapon4,
    &key_weapon5,
    &key_weapon6,
    &key_weapon7,
    &key_weapon8
};

// Set to -1 or +1 to switch to the previous or next weapon.


// Used for prev/next weapon keys.

static const struct
{
    weapontype_t weapon;
    weapontype_t weapon_num;
} weapon_order_table[] = {
    { wp_fist,            wp_fist },
    { wp_chainsaw,        wp_fist },
    { wp_pistol,          wp_pistol },
    { wp_shotgun,         wp_shotgun },
    { wp_supershotgun,    wp_shotgun },
    { wp_chaingun,        wp_chaingun },
    { wp_missile,         wp_missile },
    { wp_plasma,          wp_plasma },
    { wp_bfg,             wp_bfg }
};

#define SLOWTURNTICS	6 
 
#define NUMKEYS		256 
#define MAX_JOY_BUTTONS 20

 

// mouse values are used once 


// joystick values are repeated 
 
 
#define	BODYQUESIZE	32

mobj_t*		bodyque[BODYQUESIZE]; 
 
int             vanilla_savegame_limit = 1;
int             vanilla_demo_limit = 1;
 
int G_CmdChecksum (ticcmd_t* cmd) 
{ 
    size_t		i;
    int		sum = 0; 
	 
    for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++) 
	sum += ((int *)cmd)[i]; 
		 
    return sum; 
} 

static boolean WeaponSelectable(data_t* data, weapontype_t weapon)
{
    // Can't select the super shotgun in Doom 1.

    if (weapon == wp_supershotgun && logical_gamemission == doom)
    {
        return false;
    }

    // These weapons aren't available in shareware.

    if ((weapon == wp_plasma || weapon == wp_bfg)
     && gamemission == doom && gamemode == shareware)
    {
        return false;
    }

    // Can't select a weapon if we don't own it.

    if (!data->players[data->consoleplayer].weaponowned[weapon])
    {
        return false;
    }

    // Can't select the fist if we have the chainsaw, unless
    // we also have the berserk pack.

    if (weapon == wp_fist
     && data->players[data->consoleplayer].weaponowned[wp_chainsaw]
     && !data->players[data->consoleplayer].powers[pw_strength])
    {
        return false;
    }

    return true;
}

static int G_NextWeapon(data_t* data, int direction)
{
    weapontype_t weapon;
    int start_i, i;

    // Find index in the table.

    if (data->players[data->consoleplayer].pendingweapon == wp_nochange)
    {
        weapon = data->players[data->consoleplayer].readyweapon;
    }
    else
    {
        weapon = data->players[data->consoleplayer].pendingweapon;
    }

    for (i=0; i<arrlen(weapon_order_table); ++i)
    {
        if (weapon_order_table[i].weapon == weapon)
        {
            break;
        }
    }

    // Switch weapon. Don't loop forever.
    start_i = i;
    do
    {
        i += direction;
        i = (i + arrlen(weapon_order_table)) % arrlen(weapon_order_table);
    } while (i != start_i && !WeaponSelectable(data, weapon_order_table[i].weapon));

    return weapon_order_table[i].weapon_num;
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 
void G_BuildTiccmd (data_t* data, ticcmd_t* cmd, int maketic) 
{ 
    int		i; 
    boolean	strafe;
    boolean	bstrafe; 
    int		speed;
    int		tspeed; 
    int		forward;
    int		side;

    memset(cmd, 0, sizeof(ticcmd_t));

    cmd->consistancy = 
	data->consistancy[data->consoleplayer][maketic%BACKUPTICS]; 
 
    strafe = data->gamekeydown[key_strafe] || data->mousebuttons[mousebstrafe] 
	|| data->joybuttons[joybstrafe]; 

    // fraggle: support the old "joyb_speed = 31" hack which
    // allowed an autorun effect

    speed = key_speed >= NUMKEYS
         || joybspeed >= MAX_JOY_BUTTONS
         || data->gamekeydown[key_speed] 
         || data->joybuttons[joybspeed];
 
    forward = side = 0;
    
    // use two stage accelerative turning
    // on the keyboard and joystick
    if (data->joyxmove < 0
	|| data->joyxmove > 0  
	|| data->gamekeydown[key_right]
	|| data->gamekeydown[key_left]) 
	data->turnheld += ticdup; 
    else 
	data->turnheld = 0; 

    if (data->turnheld < SLOWTURNTICS) 
	tspeed = 2;             // slow turn 
    else 
	tspeed = speed;
    
    // let movement keys cancel each other out
    if (strafe) 
    { 
	if (data->gamekeydown[key_right]) 
	{
	    // fprintf(stderr, "strafe right\n");
	    side += sidemove[speed]; 
	}
	if (data->gamekeydown[key_left]) 
	{
	    //	fprintf(stderr, "strafe left\n");
	    side -= sidemove[speed]; 
	}
	if (data->joyxmove > 0) 
	    side += sidemove[speed]; 
	if (data->joyxmove < 0) 
	    side -= sidemove[speed]; 
 
    } 
    else 
    { 
	if (data->gamekeydown[key_right]) 
	    cmd->angleturn -= angleturn[tspeed]; 
	if (data->gamekeydown[key_left]) 
	    cmd->angleturn += angleturn[tspeed]; 
	if (data->joyxmove > 0) 
	    cmd->angleturn -= angleturn[tspeed]; 
	if (data->joyxmove < 0) 
	    cmd->angleturn += angleturn[tspeed]; 
    } 
 
    if (data->gamekeydown[key_up]) 
    {
	// fprintf(stderr, "up\n");
	forward += forwardmove[speed]; 
    }
    if (data->gamekeydown[key_down]) 
    {
	// fprintf(stderr, "down\n");
	forward -= forwardmove[speed]; 
    }

    if (data->joyymove < 0) 
        forward += forwardmove[speed]; 
    if (data->joyymove > 0) 
        forward -= forwardmove[speed]; 

    if (data->gamekeydown[key_strafeleft]
     || data->joybuttons[joybstrafeleft]
     || data->mousebuttons[mousebstrafeleft]
     || data->joystrafemove < 0)
    {
        side -= sidemove[speed];
    }

    if (data->gamekeydown[key_straferight]
     || data->joybuttons[joybstraferight]
     || data->mousebuttons[mousebstraferight]
     || data->joystrafemove > 0)
    {
        side += sidemove[speed]; 
    }

    // buttons
    cmd->chatchar = HU_dequeueChatChar(); 
 
    if (data->gamekeydown[key_fire] || data->mousebuttons[mousebfire] 
	|| data->joybuttons[joybfire]) 
	cmd->buttons |= BT_ATTACK; 
 
    if (data->gamekeydown[key_use]
     || data->joybuttons[joybuse]
     || data->mousebuttons[mousebuse])
    { 
	cmd->buttons |= BT_USE;
	// clear double clicks if hit use button 
	data->dclicks = 0;                   
    } 

    // If the previous or next weapon button is pressed, the
    // data->next_weapon variable is set to change weapons when
    // we generate a ticcmd.  Choose a new weapon.

    if (data->gamestate == GS_LEVEL && data->next_weapon != 0)
    {
        i = G_NextWeapon(data, data->next_weapon);
        cmd->buttons |= BT_CHANGE;
        cmd->buttons |= i << BT_WEAPONSHIFT;
    }
    else
    {
        // Check weapon keys.

        for (i=0; i<arrlen(weapon_keys); ++i)
        {
            int key = *weapon_keys[i];

            if (data->gamekeydown[key])
            {
                cmd->buttons |= BT_CHANGE;
                cmd->buttons |= i<<BT_WEAPONSHIFT;
                break;
            }
        }
    }

    data->next_weapon = 0;

    // mouse
    if (data->mousebuttons[mousebforward]) 
    {
	forward += forwardmove[speed];
    }
    if (data->mousebuttons[mousebbackward])
    {
        forward -= forwardmove[speed];
    }

    if (dclick_use)
    {
        // forward double click
        if (data->mousebuttons[mousebforward] != data->dclickstate && data->dclicktime > 1 ) 
        { 
            data->dclickstate = data->mousebuttons[mousebforward]; 
            if (data->dclickstate) 
                data->dclicks++; 
            if (data->dclicks == 2) 
            { 
                cmd->buttons |= BT_USE; 
                data->dclicks = 0; 
            } 
            else 
                data->dclicktime = 0; 
        } 
        else 
        { 
            data->dclicktime += ticdup; 
            if (data->dclicktime > 20) 
            { 
                data->dclicks = 0; 
                data->dclickstate = 0; 
            } 
        }
        
        // strafe double click
        bstrafe =
            data->mousebuttons[mousebstrafe] 
            || data->joybuttons[joybstrafe]; 
        if (bstrafe != data->dclickstate2 && data->dclicktime2 > 1 ) 
        { 
            data->dclickstate2 = bstrafe; 
            if (data->dclickstate2) 
                data->dclicks2++; 
            if (data->dclicks2 == 2) 
            { 
                cmd->buttons |= BT_USE; 
                data->dclicks2 = 0; 
            } 
            else 
                data->dclicktime2 = 0; 
        } 
        else 
        { 
            data->dclicktime2 += ticdup; 
            if (data->dclicktime2 > 20) 
            { 
                data->dclicks2 = 0; 
                data->dclickstate2 = 0; 
            } 
        } 
    }

    forward += data->mousey; 

    if (strafe) 
	side += data->mousex*2; 
    else 
	cmd->angleturn -= data->mousex*0x8; 

    if (data->mousex == 0)
    {
        // No movement in the previous frame

        testcontrols_mousespeed = 0;
    }
    
    data->mousex = data->mousey = 0; 
	 
    if (forward > MAXPLMOVE) 
	forward = MAXPLMOVE; 
    else if (forward < -MAXPLMOVE) 
	forward = -MAXPLMOVE; 
    if (side > MAXPLMOVE) 
	side = MAXPLMOVE; 
    else if (side < -MAXPLMOVE) 
	side = -MAXPLMOVE; 
 
    cmd->forwardmove += forward; 
    cmd->sidemove += side;
    
    // special buttons
    if (data->sendpause) 
    { 
	data->sendpause = false; 
	cmd->buttons = BT_SPECIAL | BTS_PAUSE; 
    } 
 
    if (data->sendsave) 
    { 
	data->sendsave = false; 
	cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (data->savegameslot<<BTS_SAVESHIFT); 
    } 

    // low-res turning

    if (data->lowres_turn)
    {
        static signed short carry = 0;
        signed short desired_angleturn;

        desired_angleturn = cmd->angleturn + carry;

        // round angleturn to the nearest 256 unit boundary
        // for recording demos with single byte values for turn

        cmd->angleturn = (desired_angleturn + 128) & 0xff00;

        // Carry forward the error from the reduced resolution to the
        // next tic, so that successive small movements can accumulate.

        carry = desired_angleturn - cmd->angleturn;
    }
} 
 

//
// G_DoLoadLevel 
//
void G_DoLoadLevel (data_t* data)
{ 
    int             i; 

    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.

    skyflatnum = R_FlatNumForName(DEH_String(SKYFLATNAME));

    // The "Sky never changes in Doom II" bug was fixed in
    // the id Anthology version of doom2.exe for Final Doom.
    if ((gamemode == commercial)
     && (gameversion == exe_final2 || gameversion == exe_chex))
    {
        char *skytexturename;

        if (data->gamemap < 12)
        {
            skytexturename = "SKY1";
        }
        else if (data->gamemap < 21)
        {
            skytexturename = "SKY2";
        }
        else
        {
            skytexturename = "SKY3";
        }

        skytexturename = DEH_String(skytexturename);

        skytexture = R_TextureNumForName(skytexturename);
    }

    data->levelstarttic = data->gametic;        // for time calculation
    
    if (data->wipegamestate == GS_LEVEL) 
	data->wipegamestate = -1;             // force a wipe 

    data->gamestate = GS_LEVEL; 

    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
	data->turbodetected[i] = false;
	if (data->playeringame[i] && data->players[i].playerstate == PST_DEAD) 
	    data->players[i].playerstate = PST_REBORN; 
	memset (data->players[i].frags,0,sizeof(data->players[i].frags)); 
    } 
		 
    P_SetupLevel (data, data->gameepisode, data->gamemap, 0, data->gameskill);
    data->displayplayer = data->consoleplayer;		// view the guy you are playing    
    data->gameaction = ga_nothing; 
    Z_CheckHeap ();
    
    // clear cmd building stuff

    memset (data->gamekeydown, 0, sizeof(data->gamekeydown));
    data->joyxmove = data->joyymove = data->joystrafemove = 0;
    data->mousex = data->mousey = 0;
    data->sendpause = data->sendsave = data->paused = false;
    memset(data->mousearray, 0, sizeof(data->mousearray));
    memset(data->joyarray, 0, sizeof(data->joyarray));

    if (testcontrols)
    {
        data->players[data->consoleplayer].message = "Press escape to quit.";
    }
} 

static void SetJoyButtons(data_t* data, unsigned int buttons_mask)
{
    int i;

    for (i=0; i<MAX_JOY_BUTTONS; ++i)
    {
        int button_on = (buttons_mask & (1 << i)) != 0;

        // Detect button press:

        if (!data->joybuttons[i] && button_on)
        {
            // Weapon cycling:

            if (i == joybprevweapon)
            {
                data->next_weapon = -1;
            }
            else if (i == joybnextweapon)
            {
                data->next_weapon = 1;
            }
        }

        data->joybuttons[i] = button_on;
    }
}

static void SetMouseButtons(data_t* data, unsigned int buttons_mask)
{
    int i;

    for (i=0; i<MAX_MOUSE_BUTTONS; ++i)
    {
        unsigned int button_on = (buttons_mask & (1 << i)) != 0;

        // Detect button press:

        if (!data->mousebuttons[i] && button_on)
        {
            if (i == mousebprevweapon)
            {
                data->next_weapon = -1;
            }
            else if (i == mousebnextweapon)
            {
                data->next_weapon = 1;
            }
        }

	data->mousebuttons[i] = button_on;
    }
}

//
// G_Responder  
// Get info needed to make ticcmd_ts for the data->players.
// 
boolean G_Responder (data_t* data, event_t* ev) 
{ 
    // allow spy mode changes even during the demo
    if (data->gamestate == GS_LEVEL && ev->type == ev_keydown 
     && ev->data1 == key_spy && (data->singledemo || !data->deathmatch) )
    {
	// spy mode 
	do 
	{ 
	    data->displayplayer++; 
	    if (data->displayplayer == MAXPLAYERS) 
		data->displayplayer = 0; 
	} while (!data->playeringame[data->displayplayer] && data->displayplayer != data->consoleplayer); 
	return true; 
    }
    
    // any other key pops up menu if in demos
    if (data->gameaction == ga_nothing && !data->singledemo && 
	(data->demoplayback || data->gamestate == GS_DEMOSCREEN) 
	) 
    { 
	if (ev->type == ev_keydown ||  
	    (ev->type == ev_mouse && ev->data1) || 
	    (ev->type == ev_joystick && ev->data1) ) 
	{ 
	    M_StartControlPanel (data); 
	    return true; 
	} 
	return false; 
    } 

    if (data->gamestate == GS_LEVEL) 
    { 
#if 0 
	if (devparm && ev->type == ev_keydown && ev->data1 == ';') 
	{ 
	    G_DeathMatchSpawnPlayer (data, 0); 
	    return true; 
	} 
#endif 
	if (HU_Responder (data, ev)) 
	    return true;	// chat ate the event 
	if (ST_Responder (data, ev)) 
	    return true;	// status window ate it 
	if (AM_Responder (data, ev)) 
	    return true;	// automap ate it 
    } 
	 
    if (data->gamestate == GS_FINALE) 
    { 
	if (F_Responder (data, ev)) 
	    return true;	// finale ate the event 
    } 

    if (testcontrols && ev->type == ev_mouse)
    {
        // If we are invoked by setup to test the controls, save the 
        // mouse speed so that we can display it on-screen.
        // Perform a low pass filter on this so that the thermometer 
        // appears to move smoothly.

        testcontrols_mousespeed = abs(ev->data2);
    }

    // If the next/previous weapon keys are pressed, set the data->next_weapon
    // variable to change weapons when the next ticcmd is generated.

    if (ev->type == ev_keydown && ev->data1 == key_prevweapon)
    {
        data->next_weapon = -1;
    }
    else if (ev->type == ev_keydown && ev->data1 == key_nextweapon)
    {
        data->next_weapon = 1;
    }

    switch (ev->type) 
    { 
      case ev_keydown: 
	if (ev->data1 == key_pause) 
	{ 
	    data->sendpause = true; 
	}
        else if (ev->data1 <NUMKEYS) 
        {
	    data->gamekeydown[ev->data1] = true; 
        }

	return true;    // eat key down events 
 
      case ev_keyup: 
	if (ev->data1 <NUMKEYS) 
	    data->gamekeydown[ev->data1] = false; 
	return false;   // always let key up events filter down 
		 
      case ev_mouse: 
        SetMouseButtons(data, ev->data1);
	data->mousex = ev->data2*(mouseSensitivity+5)/10; 
	data->mousey = ev->data3*(mouseSensitivity+5)/10; 
	return true;    // eat events 
 
      case ev_joystick: 
        SetJoyButtons(data, ev->data1);
	data->joyxmove = ev->data2; 
	data->joyymove = ev->data3; 
        data->joystrafemove = ev->data4;
	return true;    // eat events 
 
      default: 
	break; 
    } 
 
    return false; 
} 
 
 
 
//
// G_Ticker
// Make ticcmd_ts for the data->players.
//
void G_Ticker (data_t* data) 
{ 
    int		i;
    int		buf; 
    ticcmd_t*	cmd;
    
    // do player reborns if needed
    for (i=0 ; i<MAXPLAYERS ; i++) 
	if (data->playeringame[i] && data->players[i].playerstate == PST_REBORN) 
	    G_DoReborn (data, i);
    
    // do things to change the game state
    while (data->gameaction != ga_nothing) 
    { 
	switch (data->gameaction) 
	{ 
	  case ga_loadlevel: 
	    G_DoLoadLevel (data);
	    break; 
	  case ga_newgame: 
	    G_DoNewGame (data);
	    break; 
	  case ga_loadgame: 
	    G_DoLoadGame (data);
	    break; 
	  case ga_savegame: 
	    G_DoSaveGame (data);
	    break; 
	  case ga_playdemo: 
	    G_DoPlayDemo (data);
	    break; 
	  case ga_completed: 
	    G_DoCompleted (data);
	    break; 
	  case ga_victory: 
	    F_StartFinale (data); 
	    break; 
	  case ga_worlddone: 
	    G_DoWorldDone (data);
	    break; 
	  case ga_screenshot: 
	    V_ScreenShot("DOOM%02i.%s"); 
            data->players[data->consoleplayer].message = DEH_String("screen shot");
	    data->gameaction = ga_nothing; 
	    break; 
	  case ga_nothing: 
	    break; 
	} 
    }
    
    // get commands, check data->consistancy,
    // and build new data->consistancy check
    buf = (data->gametic/ticdup)%BACKUPTICS; 
 
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (data->playeringame[i]) 
	{ 
	    cmd = &data->players[i].cmd; 

	    memcpy(cmd, &data->netcmds[i], sizeof(ticcmd_t));

	    if (data->demoplayback) 
		G_ReadDemoTiccmd (data, cmd);
	    if (data->demorecording) 
		G_WriteDemoTiccmd (data, cmd);
	    
	    // check for turbo cheats

            // check ~ 4 seconds whether to display the turbo message. 
            // store if the turbo threshold was exceeded in any tics
            // over the past 4 seconds.  offset the checking period
            // for each player so messages are not displayed at the
            // same time.

            if (cmd->forwardmove > TURBOTHRESHOLD)
            {
                data->turbodetected[i] = true;
            }

            if ((data->gametic & 31) == 0 
             && ((data->gametic >> 5) % MAXPLAYERS) == i
             && data->turbodetected[i])
            {
                static char turbomessage[80];
                extern char *player_names[4];
                M_snprintf(turbomessage, sizeof(turbomessage),
                           "%s is turbo!", player_names[i]);
                data->players[data->consoleplayer].message = turbomessage;
                data->turbodetected[i] = false;
            }

	    if (data->netgame && !data->netdemo && !(data->gametic%ticdup) ) 
	    { 
		if (data->gametic > BACKUPTICS 
		    && data->consistancy[i][buf] != cmd->consistancy) 
		{ 
		    I_Error (NULL, "consistency failure (%i should be %i)",
			     cmd->consistancy, data->consistancy[i][buf]); 
		} 
		if (data->players[i].mo) 
		    data->consistancy[i][buf] = data->players[i].mo->x; 
		else 
		    data->consistancy[i][buf] = rndindex; 
	    } 
	}
    }
    
    // check for special buttons
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (data->playeringame[i]) 
	{ 
	    if (data->players[i].cmd.buttons & BT_SPECIAL) 
	    { 
		switch (data->players[i].cmd.buttons & BT_SPECIALMASK) 
		{ 
		  case BTS_PAUSE: 
		    data->paused ^= 1; 
		    if (data->paused) 
			S_PauseSound(data); 
		    else 
			S_ResumeSound(data); 
		    break; 
					 
		  case BTS_SAVEGAME:
		    if (!data->savedescription[0]) 
                    {
                        M_StringCopy(data->savedescription, "NET GAME",
                                     sizeof(data->savedescription));
                    }

		    data->savegameslot =  
			(data->players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT; 
		    data->gameaction = ga_savegame; 
		    break; 
		} 
	    } 
	}
    }

    // Have we just finished displaying an intermission screen?

    if (oldgamestate == GS_INTERMISSION && data->gamestate != GS_INTERMISSION)
    {
        WI_End(data);
    }

    oldgamestate = data->gamestate;
    
    // do main actions
    switch (data->gamestate) 
    { 
      case GS_LEVEL: 
	P_Ticker (data);
	ST_Ticker (data); 
	AM_Ticker (data); 
	HU_Ticker (data);            
	break; 
	 
      case GS_INTERMISSION: 
	WI_Ticker (data); 
	break; 
			 
      case GS_FINALE: 
	F_Ticker (data); 
	break; 
 
      case GS_DEMOSCREEN: 
	D_PageTicker (data);
	break;
    }        
} 
 
 
//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer (data_t* data, int player)
{
    // clear everything else to defaults
    G_PlayerReborn (data, player);
}
 
 

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel (data_t* data, int player) 
{ 
    player_t*	p; 
	 
    p = &data->players[player]; 
	 
    memset (p->powers, 0, sizeof (p->powers)); 
    memset (p->cards, 0, sizeof (p->cards)); 
    p->mo->flags &= ~MF_SHADOW;		// cancel invisibility 
    p->extralight = 0;			// cancel gun flashes 
    p->fixedcolormap = 0;		// cancel ir gogles 
    p->damagecount = 0;			// no palette changes 
    p->bonuscount = 0; 
} 
 

//
// G_PlayerReborn
// Called after a player dies 
// almost everything is cleared and initialized 
//
void G_PlayerReborn (data_t* data, int player)
{ 
    player_t*	p; 
    int		i; 
    int		frags[MAXPLAYERS]; 
    int		killcount;
    int		itemcount;
    int		secretcount; 
	 
    memcpy (frags,data->players[player].frags,sizeof(frags)); 
    killcount = data->players[player].killcount; 
    itemcount = data->players[player].itemcount; 
    secretcount = data->players[player].secretcount; 
	 
    p = &data->players[player]; 
    memset (p, 0, sizeof(*p)); 
 
    memcpy (data->players[player].frags, frags, sizeof(data->players[player].frags)); 
    data->players[player].killcount = killcount; 
    data->players[player].itemcount = itemcount; 
    data->players[player].secretcount = secretcount; 
 
    p->usedown = p->attackdown = true;	// don't do anything immediately 
    p->playerstate = PST_LIVE;       
    p->health = deh_initial_health;     // Use dehacked value
    p->readyweapon = p->pendingweapon = wp_pistol; 
    p->weaponowned[wp_fist] = true; 
    p->weaponowned[wp_pistol] = true; 
    p->ammo[am_clip] = deh_initial_bullets; 
	 
    for (i=0 ; i<NUMAMMO ; i++) 
	p->maxammo[i] = maxammo[i]; 
		 
}

//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
void P_SpawnPlayer (data_t* data, mapthing_t* mthing);
 
boolean
G_CheckSpot
( data_t* data,
  int		playernum,
  mapthing_t*	mthing ) 
{ 
    fixed_t		x;
    fixed_t		y; 
    subsector_t*	ss; 
    mobj_t*		mo; 
    int			i;
	
    if (!data->players[playernum].mo)
    {
	// first spawn of level, before corpses
	for (i=0 ; i<playernum ; i++)
	    if (data->players[i].mo->x == mthing->x << FRACBITS
		&& data->players[i].mo->y == mthing->y << FRACBITS)
		return false;	
	return true;
    }
		
    x = mthing->x << FRACBITS; 
    y = mthing->y << FRACBITS; 
	 
    if (!P_CheckPosition (data, data->players[playernum].mo, x, y) )
	return false; 
 
    // flush an old corpse if needed 
    if (data->bodyqueslot >= BODYQUESIZE) 
	P_RemoveMobj (data, bodyque[data->bodyqueslot%BODYQUESIZE]);
    bodyque[data->bodyqueslot%BODYQUESIZE] = data->players[playernum].mo; 
    data->bodyqueslot++; 

    // spawn a teleport fog
    ss = R_PointInSubsector (x,y);


    // The code in the released source looks like this:
    //
    //    an = ( ANG45 * (((unsigned int) mthing->angle)/45) )
    //         >> ANGLETOFINESHIFT;
    //    mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an]
    //                     , ss->sector->floorheight
    //                     , MT_TFOG);
    //
    // But 'an' can be a signed value in the DOS version. This means that
    // we get a negative index and the lookups into finecosine/finesine
    // end up dereferencing values in finetangent[].
    // A player spawning on a data->deathmatch start facing directly west spawns
    // "silently" with no spawn fog. Emulate this.
    //
    // This code is imported from PrBoom+.

    {
        fixed_t xa, ya;
        signed int an;

        // This calculation overflows in Vanilla Doom, but here we deliberately
        // avoid integer overflow as it is undefined behavior, so the value of
        // 'an' will always be positive.
        an = (ANG45 >> ANGLETOFINESHIFT) * ((signed int) mthing->angle / 45);

        switch (an)
        {
            case 4096:  // -4096:
                xa = finetangent[2048];    // finecosine[-4096]
                ya = finetangent[0];       // finesine[-4096]
                break;
            case 5120:  // -3072:
                xa = finetangent[3072];    // finecosine[-3072]
                ya = finetangent[1024];    // finesine[-3072]
                break;
            case 6144:  // -2048:
                xa = finesine[0];          // finecosine[-2048]
                ya = finetangent[2048];    // finesine[-2048]
                break;
            case 7168:  // -1024:
                xa = finesine[1024];       // finecosine[-1024]
                ya = finetangent[3072];    // finesine[-1024]
                break;
            case 0:
            case 1024:
            case 2048:
            case 3072:
                xa = finecosine[an];
                ya = finesine[an];
                break;
            default:
                I_Error(data, "G_CheckSpot: unexpected angle %d\n", an);
                xa = ya = 0;
                break;
        }
        mo = P_SpawnMobj(data, x + 20 * xa, y + 20 * ya,
                         ss->sector->floorheight, MT_TFOG);
    }

    if (data->players[data->consoleplayer].viewz != 1) 
	S_StartSound(data, mo, sfx_telept);	// don't start sound on first frame 
 
    return true; 
} 


//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//
void G_DeathMatchSpawnPlayer (data_t* data, int playernum)
{ 
    int             i,j; 
    int				selections; 
	 
    selections = deathmatch_p - deathmatchstarts; 
    if (selections < 4) 
	I_Error (data, "Only %i data->deathmatch spots, 4 required", selections);
 
    for (j=0 ; j<20 ; j++) 
    { 
	i = P_Random (data) % selections; 
	if (G_CheckSpot (data, playernum, &deathmatchstarts[i]) )
	{ 
	    deathmatchstarts[i].type = playernum+1; 
	    P_SpawnPlayer (data, &deathmatchstarts[i]);
	    return; 
	} 
    } 
 
    // no good spot, so the player will probably get stuck 
    P_SpawnPlayer (data, &playerstarts[playernum]);
} 

//
// G_DoReborn 
// 
void G_DoReborn (data_t* data, int playernum)
{ 
    int                             i; 
	 
    if (!data->netgame)
    {
	// reload the level from scratch
	data->gameaction = ga_loadlevel;  
    }
    else 
    {
	// respawn at the start

	// first dissasociate the corpse 
	data->players[playernum].mo->player = NULL;   
		 
	// spawn at random spot if in death match 
	if (data->deathmatch) 
	{ 
	    G_DeathMatchSpawnPlayer (data, playernum);
	    return; 
	} 
		 
	if (G_CheckSpot (data, playernum, &playerstarts[playernum]) )
	{ 
	    P_SpawnPlayer (data, &playerstarts[playernum]);
	    return; 
	}
	
	// try to spawn at one of the other data->players spots 
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (G_CheckSpot (data, playernum, &playerstarts[i]) )
	    { 
		playerstarts[i].type = playernum+1;	// fake as other player 
		P_SpawnPlayer (data, &playerstarts[i]);
		playerstarts[i].type = i+1;		// restore 
		return; 
	    }	    
	    // he's going to be inside something.  Too bad.
	}
	P_SpawnPlayer (data, &playerstarts[playernum]);
    } 
} 
 
 
void G_ScreenShot (data_t* data) 
{ 
    data->gameaction = ga_screenshot; 
} 
 


// DOOM Par Times
int pars[4][10] = 
{ 
    {0}, 
    {0,30,75,120,90,165,180,180,30,165}, 
    {0,90,90,90,120,90,360,240,30,170}, 
    {0,90,45,90,150,90,90,165,30,135} 
}; 

// DOOM II Par Times
int cpars[32] =
{
    30,90,120,120,90,150,120,120,270,90,	//  1-10
    210,150,150,150,210,150,420,150,210,150,	// 11-20
    240,150,180,150,150,300,330,420,300,180,	// 21-30
    120,30					// 31-32
};
 

//
// G_DoCompleted 
//
boolean		secretexit; 
extern char*	pagename; 
 
void G_ExitLevel (data_t* data) 
{ 
    secretexit = false; 
    data->gameaction = ga_completed; 
} 

// Here's for the german edition.
void G_SecretExitLevel (data_t* data) 
{ 
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ( (gamemode == commercial)
      && (W_CheckNumForName("map31")<0))
	secretexit = false;
    else
	secretexit = true; 
    data->gameaction = ga_completed; 
} 
 
void G_DoCompleted (data_t* data)
{ 
    int             i; 
	 
    data->gameaction = ga_nothing; 
 
    for (i=0 ; i<MAXPLAYERS ; i++) 
	if (data->playeringame[i]) 
	    G_PlayerFinishLevel (data, i);        // take away cards and stuff 
	 
    if (data->automapactive) 
	AM_Stop (data); 
	
    if (gamemode != commercial)
    {
        // Chex Quest ends after 5 levels, rather than 8.

        if (gameversion == exe_chex)
        {
            if (data->gamemap == 5)
            {
                data->gameaction = ga_victory;
                return;
            }
        }
        else
        {
            switch(data->gamemap)
            {
              case 8:
                data->gameaction = ga_victory;
                return;
              case 9: 
                for (i=0 ; i<MAXPLAYERS ; i++) 
                    data->players[i].didsecret = true; 
                break;
            }
        }
    }

//#if 0  Hmmm - why?
    if ( (data->gamemap == 8)
	 && (gamemode != commercial) ) 
    {
	// victory 
	data->gameaction = ga_victory; 
	return; 
    } 
	 
    if ( (data->gamemap == 9)
	 && (gamemode != commercial) ) 
    {
	// exit secret level 
	for (i=0 ; i<MAXPLAYERS ; i++) 
	    data->players[i].didsecret = true; 
    } 
//#endif
    
	 
    data->wminfo.didsecret = data->players[data->consoleplayer].didsecret; 
    data->wminfo.epsd = data->gameepisode -1; 
    data->wminfo.last = data->gamemap -1;
    
    // data->wminfo.next is 0 biased, unlike data->gamemap
    if ( gamemode == commercial)
    {
	if (secretexit)
	    switch(data->gamemap)
	    {
	      case 15: data->wminfo.next = 30; break;
	      case 31: data->wminfo.next = 31; break;
	    }
	else
	    switch(data->gamemap)
	    {
	      case 31:
	      case 32: data->wminfo.next = 15; break;
	      default: data->wminfo.next = data->gamemap;
	    }
    }
    else
    {
	if (secretexit) 
	    data->wminfo.next = 8; 	// go to secret level 
	else if (data->gamemap == 9) 
	{
	    // returning from secret level 
	    switch (data->gameepisode) 
	    { 
	      case 1: 
		data->wminfo.next = 3; 
		break; 
	      case 2: 
		data->wminfo.next = 5; 
		break; 
	      case 3: 
		data->wminfo.next = 6; 
		break; 
	      case 4:
		data->wminfo.next = 2;
		break;
	    }                
	} 
	else 
	    data->wminfo.next = data->gamemap;          // go to next level 
    }
		 
    data->wminfo.maxkills = data->totalkills; 
    data->wminfo.maxitems = data->totalitems; 
    data->wminfo.maxsecret = data->totalsecret; 
    data->wminfo.maxfrags = 0; 

    // Set par time. Doom episode 4 doesn't have a par time, so this
    // overflows into the cpars array. It's necessary to emulate this
    // for statcheck regression testing.
    if (gamemode == commercial)
	data->wminfo.partime = TICRATE*cpars[data->gamemap-1];
    else if (data->gameepisode < 4)
	data->wminfo.partime = TICRATE*pars[data->gameepisode][data->gamemap];
    else
        data->wminfo.partime = TICRATE*cpars[data->gamemap];

    data->wminfo.pnum = data->consoleplayer; 
 
    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
	data->wminfo.plyr[i].in = data->playeringame[i]; 
	data->wminfo.plyr[i].skills = data->players[i].killcount; 
	data->wminfo.plyr[i].sitems = data->players[i].itemcount; 
	data->wminfo.plyr[i].ssecret = data->players[i].secretcount; 
	data->wminfo.plyr[i].stime = data->leveltime; 
	memcpy (data->wminfo.plyr[i].frags, data->players[i].frags 
		, sizeof(data->wminfo.plyr[i].frags)); 
    } 
 
    data->gamestate = GS_INTERMISSION; 
    data->viewactive = false; 
    data->automapactive = false; 

    StatCopy(data, &data->wminfo);
 
    WI_Start (data, &data->wminfo); 
} 


//
// G_WorldDone 
//
void G_WorldDone (data_t* data) 
{ 
    data->gameaction = ga_worlddone; 

    if (secretexit) 
	data->players[data->consoleplayer].didsecret = true; 

    if ( gamemode == commercial )
    {
	switch (data->gamemap)
	{
	  case 15:
	  case 31:
	    if (!secretexit)
		break;
	  case 6:
	  case 11:
	  case 20:
	  case 30:
	    F_StartFinale (data);
	    break;
	}
    }
} 
 
void G_DoWorldDone (data_t* data)
{        
    data->gamestate = GS_LEVEL; 
    data->gamemap = data->wminfo.next+1; 
    G_DoLoadLevel (data);
    data->gameaction = ga_nothing; 
    data->viewactive = true; 
} 
 


//
// G_InitFromSavegame
// Can be called by the startup code or the menu task. 
//
extern boolean setsizeneeded;
void R_ExecuteSetViewSize (void);

char	savename[256];

void G_LoadGame (data_t* data, char* name) 
{ 
    M_StringCopy(savename, name, sizeof(savename));
    data->gameaction = ga_loadgame; 
} 
 
#define VERSIONSIZE		16 


void G_DoLoadGame (data_t* data) 
{
    int savedleveltime;
	 
    data->gameaction = ga_nothing; 
	 
    save_stream = fopen(savename, "rb");

    if (save_stream == NULL)
    {
    	return;
    }

    savegame_error = false;

    if (!P_ReadSaveGameHeader(data))
    {
        fclose(save_stream);
        return;
    }

    savedleveltime = data->leveltime;
    
    // load a base level 
    G_InitNew (data, data->gameskill, data->gameepisode, data->gamemap);
 
    data->leveltime = savedleveltime;

    // dearchive all the modifications
    P_UnArchivePlayers (data);
    P_UnArchiveWorld (data);
    P_UnArchiveThinkers (data);
    P_UnArchiveSpecials (data);
 
    if (!P_ReadSaveGameEOF(data))
	I_Error (data, "Bad savegame");

    fclose(save_stream);
    
    if (setsizeneeded)
    	R_ExecuteSetViewSize ();
    
    // draw the pattern into the back screen
    R_FillBackScreen(data); 
} 
 

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string 
//
void
G_SaveGame
( data_t* data,
  int	slot,
  char*	description )
{
    data->savegameslot = slot;
    M_StringCopy(data->savedescription, description, sizeof(data->savedescription));
    data->sendsave = true;
}

void G_DoSaveGame (data_t* data)
{ 
    char *savegame_file;
    char *temp_savegame_file;
    char *recovery_savegame_file;

    recovery_savegame_file = NULL;
    temp_savegame_file = P_TempSaveGameFile(data);
    savegame_file = P_SaveGameFile(data, data->savegameslot);

    // Open the savegame file for writing.  We write to a temporary file
    // and then rename it at the end if it was successfully written.
    // This prevents an existing savegame from being overwritten by 
    // a corrupted one, or if a savegame buffer overrun occurs.
    save_stream = fopen(temp_savegame_file, "wb");

    if (save_stream == NULL)
    {
        // Failed to save the game, so we're going to have to abort. But
        // to be nice, save to somewhere else before we call I_Error().
        recovery_savegame_file = M_TempFile("recovery.dsg");
        save_stream = fopen(recovery_savegame_file, "wb");
        if (save_stream == NULL)
        {
            I_Error(data, "Failed to open either '%s' or '%s' to write savegame.",
                    temp_savegame_file, recovery_savegame_file);
        }
    }

    savegame_error = false;

    P_WriteSaveGameHeader(data, data->savedescription);
 
    P_ArchivePlayers (data);
    P_ArchiveWorld (data);
    P_ArchiveThinkers (data);
    P_ArchiveSpecials (data);
	 
    P_WriteSaveGameEOF(data);
	 
    // Enforce the same savegame size limit as in Vanilla Doom, 
    // except if the vanilla_savegame_limit setting is turned off.

    if (vanilla_savegame_limit && ftell(save_stream) > SAVEGAMESIZE)
    {
        I_Error (data, "Savegame buffer overrun");
    }
    
    // Finish up, close the savegame file.

    fclose(save_stream);

    if (recovery_savegame_file != NULL)
    {
        // We failed to save to the normal location, but we wrote a
        // recovery file to the temp directory. Now we can bomb out
        // with an error.
        I_Error(data, "Failed to open savegame file '%s' for writing.\n"
                "But your game has been saved to '%s' for recovery.",
                temp_savegame_file, recovery_savegame_file);
    }

    // Now rename the temporary savegame file to the actual savegame
    // file, overwriting the old savegame if there was one there.

    remove(savegame_file);
    rename(temp_savegame_file, savegame_file);
    
    data->gameaction = ga_nothing;
    M_StringCopy(data->savedescription, "", sizeof(data->savedescription));

    data->players[data->consoleplayer].message = DEH_String(GGSAVED);

    // draw the pattern into the back screen
    R_FillBackScreen(data);	
} 
 

//
// G_InitNew
// Can be called by the startup code or the menu task,
// data->consoleplayer, data->displayplayer, data->playeringame[] should be set. 
//
skill_t	d_skill; 
int     d_episode; 
int     d_map; 
 
void
G_DeferedInitNew
( data_t* data,
  skill_t	skill,
  int		episode,
  int		map) 
{ 
    d_skill = skill; 
    d_episode = episode; 
    d_map = map; 
    data->gameaction = ga_newgame; 
} 


void G_DoNewGame (data_t* data)
{
    data->demoplayback = false; 
    data->netdemo = false;
    data->netgame = false;
    data->deathmatch = false;
    data->playeringame[1] = data->playeringame[2] = data->playeringame[3] = 0;
    data->respawnparm = false;
    data->fastparm = false;
    data->nomonsters = false;
    data->consoleplayer = 0;
    G_InitNew (data, d_skill, d_episode, d_map); 
    data->gameaction = ga_nothing; 
} 


void
G_InitNew
(
  data_t* 	data,
  skill_t	skill,
  int		episode,
  int		map )
{
    char *skytexturename;
    int             i;

    if (data->paused)
    {
	data->paused = false;
	S_ResumeSound(data);
    }

    /*
    // Note: This commented-out block of code was added at some point
    // between the DOS version(s) and the Doom source release. It isn't
    // found in disassemblies of the DOS version and causes IDCLEV and
    // the -warp command line parameter to behave differently.
    // This is left here for posterity.

    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.
    if (episode < 1)
      episode = 1;

    if ( gamemode == retail )
    {
      if (episode > 4)
	episode = 4;
    }
    else if ( gamemode == shareware )
    {
      if (episode > 1)
	   episode = 1;	// only start episode 1 on shareware
    }
    else
    {
      if (episode > 3)
	episode = 3;
    }
    */

    if (skill > sk_nightmare)
	skill = sk_nightmare;

    if (gameversion >= exe_ultimate)
    {
        if (episode == 0)
        {
            episode = 4;
        }
    }
    else
    {
        if (episode < 1)
        {
            episode = 1;
        }
        if (episode > 3)
        {
            episode = 3;
        }
    }

    if (episode > 1 && gamemode == shareware)
    {
        episode = 1;
    }

    if (map < 1)
	map = 1;

    if ( (map > 9)
	 && ( gamemode != commercial) )
      map = 9;

    M_ClearRandom (data);

    if (skill == sk_nightmare || data->respawnparm )
	data->respawnmonsters = true;
    else
	data->respawnmonsters = false;

    if (data->fastparm || (skill == sk_nightmare && data->gameskill != sk_nightmare) )
    {
	for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	    states[i].tics >>= 1;
	mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
	mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
	mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
    }
    else if (skill != sk_nightmare && data->gameskill == sk_nightmare)
    {
	for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	    states[i].tics <<= 1;
	mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
	mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
	mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
    }

    // force data->players to be initialized upon first level load
    for (i=0 ; i<MAXPLAYERS ; i++)
	data->players[i].playerstate = PST_REBORN;

    data->usergame = true;                // will be set false if a demo
    data->paused = false;
    data->demoplayback = false;
    data->automapactive = false;
    data->viewactive = true;
    data->gameepisode = episode;
    data->gamemap = map;
    data->gameskill = skill;

    data->viewactive = true;

    // Set the sky to use.
    //
    // Note: This IS broken, but it is how Vanilla Doom behaves.
    // See http://doomwiki.org/wiki/Sky_never_changes_in_Doom_II.
    //
    // Because we set the sky here at the start of a game, not at the
    // start of a level, the sky texture never changes unless we
    // restore from a saved game.  This was fixed before the Doom
    // source release, but this IS the way Vanilla DOS Doom behaves.

    if (gamemode == commercial)
    {
        if (data->gamemap < 12)
            skytexturename = "SKY1";
        else if (data->gamemap < 21)
            skytexturename = "SKY2";
        else
            skytexturename = "SKY3";
    }
    else
    {
        switch (data->gameepisode)
        {
          default:
          case 1:
            skytexturename = "SKY1";
            break;
          case 2:
            skytexturename = "SKY2";
            break;
          case 3:
            skytexturename = "SKY3";
            break;
          case 4:        // Special Edition sky
            skytexturename = "SKY4";
            break;
        }
    }

    skytexturename = DEH_String(skytexturename);

    skytexture = R_TextureNumForName(skytexturename);


    G_DoLoadLevel (data);
}


//
// DEMO RECORDING 
// 
#define DEMOMARKER		0x80


void G_ReadDemoTiccmd (data_t* data, ticcmd_t* cmd)
{ 
    if (*demo_p == DEMOMARKER) 
    {
	// end of demo data stream 
	G_CheckDemoStatus (data);
	return; 
    } 
    cmd->forwardmove = ((signed char)*demo_p++); 
    cmd->sidemove = ((signed char)*demo_p++); 

    // If this is a data->longtics demo, read back in higher resolution

    if (data->longtics)
    {
        cmd->angleturn = *demo_p++;
        cmd->angleturn |= (*demo_p++) << 8;
    }
    else
    {
        cmd->angleturn = ((unsigned char) *demo_p++)<<8; 
    }

    cmd->buttons = (unsigned char)*demo_p++; 
} 

// Increase the size of the demo buffer to allow unlimited demos

static void IncreaseDemoBuffer(data_t* data)
{
    int current_length;
    byte *new_demobuffer;
    byte *new_demop;
    int new_length;

    // Find the current size

    current_length = demoend - demobuffer;
    
    // Generate a new buffer twice the size
    new_length = current_length * 2;
    
    new_demobuffer = Z_Malloc(new_length, PU_STATIC, 0);
    new_demop = new_demobuffer + (demo_p - demobuffer);

    // Copy over the old data

    memcpy(new_demobuffer, demobuffer, current_length);

    // Free the old buffer and point the demo pointers at the new buffer.

    Z_Free(demobuffer);

    demobuffer = new_demobuffer;
    demo_p = new_demop;
    demoend = demobuffer + new_length;
}

void G_WriteDemoTiccmd (data_t* data, ticcmd_t* cmd)
{ 
    byte *demo_start;

    if (data->gamekeydown[key_demo_quit])           // press q to end demo recording 
	G_CheckDemoStatus (data);

    demo_start = demo_p;

    *demo_p++ = cmd->forwardmove; 
    *demo_p++ = cmd->sidemove; 

    // If this is a data->longtics demo, record in higher resolution
 
    if (data->longtics)
    {
        *demo_p++ = (cmd->angleturn & 0xff);
        *demo_p++ = (cmd->angleturn >> 8) & 0xff;
    }
    else
    {
        *demo_p++ = cmd->angleturn >> 8; 
    }

    *demo_p++ = cmd->buttons; 

    // reset demo pointer back
    demo_p = demo_start;

    if (demo_p > demoend - 16)
    {
        if (vanilla_demo_limit)
        {
            // no more space 
            G_CheckDemoStatus (data); 
            return; 
        }
        else
        {
            // Vanilla demo limit disabled: unlimited
            // demo lengths!

            IncreaseDemoBuffer (data);
        }
    } 
	
    G_ReadDemoTiccmd (data, cmd);         // make SURE it is exactly the same 
} 
 
 
 
//
// G_RecordDemo
//
void G_RecordDemo (data_t* data, char *name)
{
    size_t demoname_size;
    int i;
    int maxsize;

    data->usergame = false;
    demoname_size = strlen(name) + 5;
    demoname = Z_Malloc(demoname_size, PU_STATIC, NULL);
    M_snprintf(demoname, demoname_size, "%s.lmp", name);
    maxsize = 0x20000;

    //!
    // @arg <size>
    // @category demo
    // @vanilla
    //
    // Specify the demo buffer size (KiB)
    //

    i = M_CheckParmWithArgs(data, "-maxdemo", 1);
    if (i)
	maxsize = atoi(data->myargv[i+1])*1024;
    demobuffer = Z_Malloc (maxsize,PU_STATIC,NULL); 
    demoend = demobuffer + maxsize;
	
    data->demorecording = true; 
} 

// Get the demo version code appropriate for the version set in gameversion.
int G_VanillaVersionCode(data_t* data)
{
    switch (gameversion)
    {
        case exe_doom_1_2:
            I_Error(data, "Doom 1.2 does not have a version code!");
        case exe_doom_1_666:
            return 106;
        case exe_doom_1_7:
            return 107;
        case exe_doom_1_8:
            return 108;
        case exe_doom_1_9:
        default:  // All other versions are variants on v1.9:
            return 109;
    }
}

void G_BeginRecording (data_t* data) 
{ 
    int             i; 

    //!
    // @category demo
    //
    // Record a high resolution "Doom 1.91" demo.
    //

    data->longtics = M_CheckParm(data, "-data->longtics") != 0;

    // If not recording a data->longtics demo, record in low res

    data->lowres_turn = !data->longtics;
    
    demo_p = demobuffer;
	
    // Save the right version code for this demo
 
    if (data->longtics)
    {
        *demo_p++ = DOOM_191_VERSION;
    }
    else
    {
        *demo_p++ = G_VanillaVersionCode(data);
    }

    *demo_p++ = data->gameskill; 
    *demo_p++ = data->gameepisode; 
    *demo_p++ = data->gamemap; 
    *demo_p++ = data->deathmatch; 
    *demo_p++ = data->respawnparm;
    *demo_p++ = data->fastparm;
    *demo_p++ = data->nomonsters;
    *demo_p++ = data->consoleplayer;
	 
    for (i=0 ; i<MAXPLAYERS ; i++) 
	*demo_p++ = data->playeringame[i]; 		 
} 
 

//
// G_PlayDemo 
//

char*	defdemoname; 
 
void G_DeferedPlayDemo (data_t* data, char* name) 
{ 
    defdemoname = name; 
    data->gameaction = ga_playdemo; 
} 

// Generate a string describing a demo version

static char *DemoVersionDescription(int version)
{
    static char resultbuf[16];

    switch (version)
    {
        case 104:
            return "v1.4";
        case 105:
            return "v1.5";
        case 106:
            return "v1.6/v1.666";
        case 107:
            return "v1.7/v1.7a";
        case 108:
            return "v1.8";
        case 109:
            return "v1.9";
        default:
            break;
    }

    // Unknown version.  Perhaps this is a pre-v1.4 IWAD?  If the version
    // byte is in the range 0-4 then it can be a v1.0-v1.2 demo.

    if (version >= 0 && version <= 4)
    {
        return "v1.0/v1.1/v1.2";
    }
    else
    {
        M_snprintf(resultbuf, sizeof(resultbuf),
                   "%i.%i (unknown)", version / 100, version % 100);
        return resultbuf;
    }
}

void G_DoPlayDemo (data_t* data)
{ 
    skill_t skill; 
    int             i, episode, map; 
    int demoversion;
	 
    data->gameaction = ga_nothing; 
    demobuffer = demo_p = W_CacheLumpName (defdemoname, PU_STATIC); 

    demoversion = *demo_p++;

    if (demoversion == G_VanillaVersionCode(data))
    {
        data->longtics = false;
    }
    else if (demoversion == DOOM_191_VERSION)
    {
        // demo recorded with cph's modified "v1.91" doom exe
        data->longtics = true;
    }
    else
    {
        char *message = "Demo is from a different game version!\n"
                        "(read %i, should be %i)\n"
                        "\n"
                        "*** You may need to upgrade your version "
                            "of Doom to v1.9. ***\n"
                        "    See: https://www.doomworld.com/classicdoom"
                                  "/info/patches.php\n"
                        "    This appears to be %s.";

        //I_Error(message, demoversion, G_VanillaVersionCode(),
        printf(message, demoversion, G_VanillaVersionCode(data),
                         DemoVersionDescription(demoversion));
    }
    
    skill = *demo_p++; 
    episode = *demo_p++; 
    map = *demo_p++; 
    data->deathmatch = *demo_p++;
    data->respawnparm = *demo_p++;
    data->fastparm = *demo_p++;
    data->nomonsters = *demo_p++;
    data->consoleplayer = *demo_p++;
	
    for (i=0 ; i<MAXPLAYERS ; i++) 
	data->playeringame[i] = *demo_p++; 

    if (data->playeringame[1] || M_CheckParm(data, "-solo-net") > 0
                        || M_CheckParm(data, "-data->netdemo") > 0)
    {
	data->netgame = true;
	data->netdemo = true;
    }

    // don't spend a lot of time in loadlevel 
    data->precache = false;
    G_InitNew (data, skill, episode, map);
    data->precache = true; 
    data->starttime = I_GetTime (data);

    data->usergame = false; 
    data->demoplayback = true; 
} 

//
// G_TimeDemo 
//
void G_TimeDemo (data_t* data, char* name) 
{
    //!
    // @vanilla 
    //
    // Disable rendering the screen entirely.
    //

    data->nodrawers = M_CheckParm (data, "-nodraw");

    data->timingdemo = true; 
    singletics = true; 

    defdemoname = name; 
    data->gameaction = ga_playdemo; 
} 
 
 
/* 
=================== 
= 
= G_CheckDemoStatus 
= 
= Called after a death or level completion to allow demos to be cleaned up 
= Returns true if a new demo loop action will take place 
=================== 
*/ 
 
boolean G_CheckDemoStatus (data_t* data) 
{ 
    int             endtime; 
	 
    if (data->timingdemo) 
    { 
        float fps;
        int realtics;

	endtime = I_GetTime (data);
        realtics = endtime - data->starttime;
        fps = ((float) data->gametic * TICRATE) / realtics;

        // Prevent recursive calls
        data->timingdemo = false;
        data->demoplayback = false;

	I_Error (NULL, "timed %i gametics in %i realtics (%f fps)",
                 data->gametic, realtics, fps);
    } 
	 
    if (data->demoplayback) 
    { 
        W_ReleaseLumpName(defdemoname);
	data->demoplayback = false; 
	data->netdemo = false;
	data->netgame = false;
	data->deathmatch = false;
	data->playeringame[1] = data->playeringame[2] = data->playeringame[3] = 0;
	data->respawnparm = false;
	data->fastparm = false;
	data->nomonsters = false;
	data->consoleplayer = 0;
        
        if (data->singledemo) 
            I_Quit (data);
        else 
            D_AdvanceDemo (data);

	return true; 
    } 
 
    if (data->demorecording) 
    { 
	*demo_p++ = DEMOMARKER; 
	M_WriteFile (demoname, demobuffer, demo_p - demobuffer); 
	Z_Free (demobuffer); 
	data->demorecording = false; 
	I_Error (NULL, "Demo %s recorded",demoname); 
    } 
	 
    return false; 
} 
 
 
 
