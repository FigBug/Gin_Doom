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
//	DOOM Network game communication and protocol,
//	all OS independend parts.
//

#include <stdlib.h>

#include "doomfeatures.h"

#include "d_main.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "w_checksum.h"
#include "w_wad.h"

#include "deh_main.h"

#include "d_loop.h"

ticcmd_t *netcmds;

// Called when a player leaves the game

static void PlayerQuitGame(data_t* data, player_t *player)
{
    static char exitmsg[80];
    unsigned int player_num;

    player_num = player - players;

    // Do this the same way as Vanilla Doom does, to allow dehacked
    // replacements of this message

    M_StringCopy(exitmsg, DEH_String("Player 1 left the game"),
                 sizeof(exitmsg));

    exitmsg[7] += player_num;

    playeringame[player_num] = false;
    players[consoleplayer].message = exitmsg;

    // TODO: check if it is sensible to do this:

    if (demorecording) 
    {
        G_CheckDemoStatus (data);
    }
}

static void RunTic(data_t* data, ticcmd_t *cmds, boolean *ingame)
{
    unsigned int i;

    // Check for player quits.

    for (i = 0; i < MAXPLAYERS; ++i)
    {
        if (!demoplayback && playeringame[i] && !ingame[i])
        {
            PlayerQuitGame(data, &players[i]);
        }
    }

    netcmds = cmds;

    // check that there are players in the game.  if not, we cannot
    // run a tic.

    if (data->d_main.advancedemo)
        D_DoAdvanceDemo (data);

    G_Ticker (data);
}

static loop_interface_t doom_loop_interface = {
    D_ProcessEvents,
    G_BuildTiccmd,
    RunTic,
    M_Ticker
};


// Load game settings from the specified structure and
// set global variables.

static void LoadGameSettings(data_t* data, net_gamesettings_t *settings)
{
    unsigned int i;

    deathmatch = settings->deathmatch;
    data->d_main.startepisode = settings->episode;
	data->d_main.startmap = settings->map;
    data->d_main.startskill = settings->skill;
    data->d_main.startloadgame = settings->loadgame;
    lowres_turn = settings->lowres_turn;
	data->d_main.nomonsters = settings->nomonsters;
	data->d_main.fastparm = settings->fast_monsters;
	data->d_main.respawnparm = settings->respawn_monsters;
    timelimit = settings->timelimit;
    consoleplayer = settings->consoleplayer;

    if (lowres_turn)
    {
        printf("NOTE: Turning resolution is reduced; this is probably "
               "because there is a client recording a Vanilla demo.\n");
    }

    for (i = 0; i < MAXPLAYERS; ++i)
    {
        playeringame[i] = i < settings->num_players;
    }
}

// Save the game settings from global variables to the specified
// game settings structure.

static void SaveGameSettings(data_t* data, net_gamesettings_t *settings)
{
    // Fill in game settings structure with appropriate parameters
    // for the new game

    settings->deathmatch = deathmatch;
    settings->episode = data->d_main.startepisode;
    settings->map = data->d_main.startmap;
    settings->skill = data->d_main.startskill;
    settings->loadgame = data->d_main.startloadgame;
    settings->gameversion = gameversion;
    settings->nomonsters = data->d_main.nomonsters;
    settings->fast_monsters = data->d_main.fastparm;
    settings->respawn_monsters = data->d_main.respawnparm;
    settings->timelimit = timelimit;

    settings->lowres_turn = M_CheckParm(data, "-record") > 0
                         && M_CheckParm(data, "-longtics") == 0;
}

static void InitConnectData(data_t* data, net_connect_data_t *connect_data)
{
    connect_data->max_players = MAXPLAYERS;
    connect_data->drone = false;

    //!
    // @category net
    //
    // Run as the left screen in three screen mode.
    //

    if (M_CheckParm(data, "-left") > 0)
    {
        viewangleoffset = ANG90;
        connect_data->drone = true;
    }

    //! 
    // @category net
    //
    // Run as the right screen in three screen mode.
    //

    if (M_CheckParm(data, "-right") > 0)
    {
        viewangleoffset = ANG270;
        connect_data->drone = true;
    }

    //
    // Connect data
    //

    // Game type fields:

    connect_data->gamemode = gamemode;
    connect_data->gamemission = gamemission;

    // Are we recording a demo? Possibly set lowres turn mode

    connect_data->lowres_turn = M_CheckParm(data, "-record") > 0
                             && M_CheckParm(data, "-longtics") == 0;

    // Read checksums of our WAD directory and dehacked information

    W_Checksum(connect_data->wad_sha1sum);

#if ORIGCODE
    DEH_Checksum(connect_data->deh_sha1sum);
#endif

    // Are we playing with the Freedoom IWAD?

    connect_data->is_freedoom = W_CheckNumForName("FREEDOOM") >= 0;
}

void D_ConnectNetGame(data_t* data)
{
    net_connect_data_t connect_data;

    InitConnectData(data, &connect_data);
    netgame = D_InitNetGame(data, &connect_data);

    //!
    // @category net
    //
    // Start the game playing as though in a netgame with a single
    // player.  This can also be used to play back single player netgame
    // demos.
    //

    if (M_CheckParm(data, "-solo-net") > 0)
    {
        netgame = true;
    }
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame (data_t* data)
{
    net_gamesettings_t settings;

    if (netgame)
    {
		data->d_main.autostart = true;
    }

    D_RegisterLoopCallbacks(data, &doom_loop_interface);

    SaveGameSettings(data, &settings);
    D_StartNetGame(data, &settings, NULL);
    LoadGameSettings(data, &settings);

    DEH_printf("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
			   data->d_main.startskill, deathmatch, data->d_main.startmap, data->d_main.startepisode);

    DEH_printf("player %i of %i (%i nodes)\n",
               consoleplayer+1, settings.num_players, settings.num_players);

    // Show players here; the server might have specified a time limit

    if (timelimit > 0 && deathmatch)
    {
        // Gross hack to work like Vanilla:

        if (timelimit == 20 && M_CheckParm(data, "-avg"))
        {
            DEH_printf("Austin Virtual Gaming: Levels will end "
                           "after 20 minutes\n");
        }
        else
        {
            DEH_printf("Levels will end after %d minute", timelimit);
            if (timelimit > 1)
                printf("s");
            printf(".\n");
        }
    }
}

