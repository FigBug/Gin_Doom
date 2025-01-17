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
//     Main loop code.
//

#include <stdlib.h>
#include <string.h>

#include "doomfeatures.h"

#include "d_event.h"
#include "d_loop.h"
#include "d_ticcmd.h"

#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "m_argv.h"
#include "m_fixed.h"

#include "net_client.h"
#include "net_gui.h"
#include "net_io.h"
#include "net_query.h"
#include "net_server.h"
#include "net_sdl.h"
#include "net_loop.h"

// The complete set of data for a particular tic.
void D_Loop_Init (data_t* data)
{
	data->d_loop.singletics = false;
	data->d_loop.skiptics = 0;
	data->d_loop.new_sync = true;
	data->d_loop.loop_interface = NULL;
}

// 35 fps clock adjusted by offsetms milliseconds

static int GetAdjustedTime(data_t* data)
{
    int time_ms;

    time_ms = I_GetTimeMS (data);

    if (data->d_loop.new_sync)
    {
	// Use the adjustments from net_client.c only if we are
	// using the new sync mode.

        time_ms += (data->d_loop.offsetms / FRACUNIT);
    }

    return (time_ms * TICRATE) / 1000;
}

static boolean BuildNewTic (data_t* data)
{
    int	gameticdiv;
    ticcmd_t cmd;

    gameticdiv = data->d_loop.gametic / data->d_loop.ticdup;

    I_StartTic (data);
	data->d_loop.loop_interface->ProcessEvents (data);

    // Always run the menu

	data->d_loop.loop_interface->RunMenu (data);

    if (drone)
    {
        // In drone mode, do not generate any ticcmds.

        return false;
    }

    if (data->d_loop.new_sync)
    {
       // If playing single player, do not allow tics to buffer
       // up very far

       if (!net_client_connected && data->d_loop.maketic - gameticdiv > 2)
           return false;

       // Never go more than ~200ms ahead

       if (data->d_loop.maketic - gameticdiv > 8)
           return false;
    }
    else
    {
       if (data->d_loop.maketic - gameticdiv >= 5)
           return false;
    }

    //printf ("mk:%i ",maketic);
    memset(&cmd, 0, sizeof(ticcmd_t));
	data->d_loop.loop_interface->BuildTiccmd(data, &cmd, data->d_loop.maketic);

#ifdef FEATURE_MULTIPLAYER

    if (net_client_connected)
    {
        NET_CL_SendTiccmd(&cmd, maketic);
    }

#endif
	data->d_loop.ticdata[data->d_loop.maketic % BACKUPTICS].cmds[data->d_loop.localplayer] = cmd;
	data->d_loop.ticdata[data->d_loop.maketic % BACKUPTICS].ingame[data->d_loop.localplayer] = true;

    ++data->d_loop.maketic;

    return true;
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int      lasttime;

void NetUpdate (data_t* data)
{
    int nowtime;
    int newtics;
    int	i;

    // If we are running with singletics (timing a demo), this
    // is all done separately.

    if (data->d_loop.singletics)
        return;

#ifdef FEATURE_MULTIPLAYER

    // Run network subsystems

    NET_CL_Run();
    NET_SV_Run();

#endif

    // check time
    nowtime = GetAdjustedTime (data) / data->d_loop.ticdup;
    newtics = nowtime - lasttime;

    lasttime = nowtime;

    if (data->d_loop.skiptics <= newtics)
    {
        newtics -= data->d_loop.skiptics;
		data->d_loop.skiptics = 0;
    }
    else
    {
		data->d_loop.skiptics -= newtics;
        newtics = 0;
    }

    // build new ticcmds for console player

    for (i=0 ; i<newtics ; i++)
    {
        if (!BuildNewTic (data))
        {
            break;
        }
    }
}

static void D_Disconnected(data_t* data)
{
    // In drone mode, the game cannot continue once disconnected.

    if (drone)
    {
        I_Error(data, "Disconnected from server in drone mode.");
    }

    // disconnected from server

    printf("Disconnected from server.\n");
}

//
// Invoked by the network engine when a complete set of ticcmds is
// available.
//

void D_ReceiveTic(data_t* data, ticcmd_t *ticcmds, boolean *players_mask)
{
    int i;

    // Disconnected from server?

    if (ticcmds == NULL && players_mask == NULL)
    {
        D_Disconnected(data);
        return;
    }

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        if (!drone && i == data->d_loop.localplayer)
        {
            // This is us.  Don't overwrite it.
        }
        else
        {
			data->d_loop.ticdata[data->d_loop.recvtic % BACKUPTICS].cmds[i] = ticcmds[i];
			data->d_loop.ticdata[data->d_loop.recvtic % BACKUPTICS].ingame[i] = players_mask[i];
        }
    }

    ++data->d_loop.recvtic;
}

//
// Start game loop
//
// Called after the screen is set but before the game starts running.
//

void D_StartGameLoop (data_t* data)
{
    lasttime = GetAdjustedTime (data) / data->d_loop.ticdup;
}

#if ORIGCODE
//
// Block until the game start message is received from the server.
//

static void BlockUntilStart(net_gamesettings_t *settings,
                            netgame_startup_callback_t callback)
{
    while (!NET_CL_GetSettings(settings))
    {
        NET_CL_Run();
        NET_SV_Run();

        if (!net_client_connected)
        {
            I_Error (NULL, "Lost connection to server");
        }

        if (callback != NULL && !callback(net_client_wait_data.ready_players,
                                          net_client_wait_data.num_players))
        {
            I_Error (NULL, "Netgame startup aborted.");
        }

        I_Sleep(data, 100);
    }
}

#endif

void D_StartNetGame(data_t* data, net_gamesettings_t *settings,
                    netgame_startup_callback_t callback)
{
#if ORIGCODE
    int i;

    offsetms = 0;
    recvtic = 0;

    settings->consoleplayer = 0;
    settings->num_players = 1;
    settings->player_classes[0] = player_class;

    //!
    // @category net
    //
    // Use new network client sync code rather than the classic
    // sync code. This is currently disabled by default because it
    // has some bugs.
    //
    if (M_CheckParm("-newsync") > 0)
        settings->new_sync = 1;
    else
        settings->new_sync = 0;

    // TODO: New sync code is not enabled by default because it's
    // currently broken. 
    //if (M_CheckParm("-oldsync") > 0)
    //    settings->new_sync = 0;
    //else
    //    settings->new_sync = 1;

    //!
    // @category net
    // @arg <n>
    //
    // Send n extra tics in every packet as insurance against dropped
    // packets.
    //

    i = M_CheckParmWithArgs("-extratics", 1);

    if (i > 0)
        settings->extratics = atoi(myargv[i+1]);
    else
        settings->extratics = 1;

    //!
    // @category net
    // @arg <n>
    //
    // Reduce the resolution of the game by a factor of n, reducing
    // the amount of network bandwidth needed.
    //

    i = M_CheckParmWithArgs("-dup", 1);

    if (i > 0)
        settings->ticdup = atoi(myargv[i+1]);
    else
        settings->ticdup = 1;

    if (net_client_connected)
    {
        // Send our game settings and block until game start is received
        // from the server.

        NET_CL_StartGame(settings);
        BlockUntilStart(settings, callback);

        // Read the game settings that were received.

        NET_CL_GetSettings(settings);
    }

    if (drone)
    {
        settings->consoleplayer = 0;
    }

    // Set the local player and playeringame[] values.

    localplayer = settings->consoleplayer;

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        local_playeringame[i] = i < settings->num_players;
    }

    // Copy settings to global variables.

    ticdup = settings->ticdup;
    new_sync = settings->new_sync;

    // TODO: Message disabled until we fix new_sync.
    //if (!new_sync)
    //{
    //    printf("Syncing netgames like Vanilla Doom.\n");
    //}
#else
    settings->consoleplayer = 0;
	settings->num_players = 1;
	settings->player_classes[0] = data->d_loop.player_class;
	settings->new_sync = 0;
	settings->extratics = 1;
	settings->ticdup = 1;

	data->d_loop.ticdup = settings->ticdup;
	data->d_loop.new_sync = settings->new_sync;
#endif
}

boolean D_InitNetGame(data_t* data, net_connect_data_t *connect_data)
{
    boolean result = false;
#ifdef FEATURE_MULTIPLAYER
    net_addr_t *addr = NULL;
    int i;
#endif

    // Call D_QuitNetGame on exit:

    I_AtExit(D_QuitNetGame, true);

	data->d_loop.player_class = connect_data->player_class;

#ifdef FEATURE_MULTIPLAYER

    //!
    // @category net
    //
    // Start a multiplayer server, listening for connections.
    //

    if (M_CheckParm("-server") > 0
     || M_CheckParm("-privateserver") > 0)
    {
        NET_SV_Init();
        NET_SV_AddModule(&net_loop_server_module);
        NET_SV_AddModule(&net_sdl_module);
        NET_SV_RegisterWithMaster();

        net_loop_client_module.InitClient();
        addr = net_loop_client_module.ResolveAddress(NULL);
    }
    else
    {
        //!
        // @category net
        //
        // Automatically search the local LAN for a multiplayer
        // server and join it.
        //

        i = M_CheckParm("-autojoin");

        if (i > 0)
        {
            addr = NET_FindLANServer();

            if (addr == NULL)
            {
                I_Error (NULL, "No server found on local LAN");
            }
        }

        //!
        // @arg <address>
        // @category net
        //
        // Connect to a multiplayer server running on the given
        // address.
        //

        i = M_CheckParmWithArgs("-connect", 1);

        if (i > 0)
        {
            net_sdl_module.InitClient();
            addr = net_sdl_module.ResolveAddress(myargv[i+1]);

            if (addr == NULL)
            {
                I_Error (NULL, "Unable to resolve '%s'\n", myargv[i+1]);
            }
        }
    }

    if (addr != NULL)
    {
        if (M_CheckParm("-drone") > 0)
        {
            connect_data->drone = true;
        }

        if (!NET_CL_Connect(addr, connect_data))
        {
            I_Error (NULL, "D_InitNetGame: Failed to connect to %s\n",
                    NET_AddrToString(addr));
        }

        printf("D_InitNetGame: Connected to %s\n", NET_AddrToString(addr));

        // Wait for launch message received from server.

        NET_WaitForLaunch();

        result = true;
    }
#endif

    return result;
}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame (data_t* data)
{
#ifdef FEATURE_MULTIPLAYER
    NET_SV_Shutdown();
    NET_CL_Disconnect();
#endif
}

static int GetLowTic(data_t* data)
{
    int lowtic;

    lowtic = data->d_loop.maketic;

#ifdef FEATURE_MULTIPLAYER
    if (net_client_connected)
    {
        if (drone || recvtic < lowtic)
        {
            lowtic = recvtic;
        }
    }
#endif

    return lowtic;
}

static int frameon;
static int frameskip[4];
static int oldnettics;

static void OldNetSync(data_t* data)
{
    unsigned int i;
    int keyplayer = -1;

    frameon++;

    // ideally maketic should be 1 - 3 tics above lowtic
    // if we are consistantly slower, speed up time

    for (i=0 ; i<NET_MAXPLAYERS ; i++)
    {
        if (data->d_loop.local_playeringame[i])
        {
            keyplayer = i;
            break;
        }
    }

    if (keyplayer < 0)
    {
        // If there are no players, we can never advance anyway

        return;
    }

    if (data->d_loop.localplayer == keyplayer)
    {
        // the key player does not adapt
    }
    else
    {
        if (data->d_loop.maketic <= data->d_loop.recvtic)
        {
            lasttime--;
            // printf ("-");
        }

        frameskip[frameon & 3] = oldnettics > data->d_loop.recvtic;
        oldnettics = data->d_loop.maketic;

        if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
        {
			data->d_loop.skiptics = 1;
            // printf ("+");
        }
    }
}

// Returns true if there are players in the game:

static boolean PlayersInGame(data_t* data)
{
    boolean result = false;
    unsigned int i;

    // If we are connected to a server, check if there are any players
    // in the game.

    if (net_client_connected)
    {
        for (i = 0; i < NET_MAXPLAYERS; ++i)
        {
            result = result || data->d_loop.local_playeringame[i];
        }
    }

    // Whether single or multi-player, unless we are running as a drone,
    // we are in the game.

    if (!drone)
    {
        result = true;
    }

    return result;
}

// When using ticdup, certain values must be cleared out when running
// the duplicate ticcmds.

static void TicdupSquash(ticcmd_set_t *set)
{
    ticcmd_t *cmd;
    unsigned int i;

    for (i = 0; i < NET_MAXPLAYERS ; ++i)
    {
        cmd = &set->cmds[i];
        cmd->chatchar = 0;
        if (cmd->buttons & BT_SPECIAL)
            cmd->buttons = 0;
    }
}

// When running in single player mode, clear all the ingame[] array
// except the local player.

static void SinglePlayerClear(data_t* data, ticcmd_set_t *set)
{
    unsigned int i;

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        if (i != data->d_loop.localplayer)
        {
            set->ingame[i] = false;
        }
    }
}

//
// TryRunTics
//

void TryRunTics (data_t* data)
{
    int	i;
    int	lowtic;
    int	entertic;
    static int oldentertics;
    int realtics;
    int	availabletics;
    int	counts;

    // get real tics
    entertic = I_GetTime (data) / data->d_loop.ticdup;
    realtics = entertic - oldentertics;
    oldentertics = entertic;

    // in singletics mode, run a single tic every time this function
    // is called.

    if (data->d_loop.singletics)
    {
        BuildNewTic (data);
    }
    else
    {
        NetUpdate (data);
    }

    lowtic = GetLowTic(data);

    availabletics = lowtic - data->d_loop.gametic / data->d_loop.ticdup;

    // decide how many tics to run

    if (data->d_loop.new_sync)
    {
		counts = availabletics;
    }
    else
    {
        // decide how many tics to run
        if (realtics < availabletics-1)
            counts = realtics+1;
        else if (realtics < availabletics)
            counts = realtics;
        else
            counts = availabletics;

        if (counts < 1)
            counts = 1;

        if (net_client_connected)
        {
            OldNetSync(data);
        }
    }

    if (counts < 1)
	counts = 1;

    // wait for new tics if needed

    while (!PlayersInGame(data) || lowtic < data->d_loop.gametic / data->d_loop.ticdup + counts)
    {
	NetUpdate (data);

        lowtic = GetLowTic(data);

	if (lowtic < data->d_loop.gametic / data->d_loop.ticdup)
	    I_Error (data, "TryRunTics: lowtic < gametic");

        // Don't stay in this loop forever.  The menu is still running,
        // so return to update the screen

	if (I_GetTime(data) / data->d_loop.ticdup - entertic > 0)
	{
	    return;
	}

        I_Sleep(data, 1);
    }

    // run the count * ticdup dics
    while (counts--)
    {
        ticcmd_set_t *set;

        if (!PlayersInGame(data))
        {
            return;
        }

        set = &data->d_loop.ticdata[(data->d_loop.gametic / data->d_loop.ticdup) % BACKUPTICS];

        if (!net_client_connected)
        {
            SinglePlayerClear(data, set);
        }

	for (i=0 ; i<data->d_loop.ticdup ; i++)
	{
            if (data->d_loop.gametic / data->d_loop.ticdup > lowtic)
                I_Error (data, "gametic>lowtic");

            memcpy(data->d_loop.local_playeringame, set->ingame, sizeof(data->d_loop.local_playeringame));

		data->d_loop.loop_interface->RunTic (data, set->cmds, set->ingame);
		data->d_loop.gametic++;

	    // modify command for duplicated tics

            TicdupSquash(set);
	}

	NetUpdate (data);	// check for new console commands
    }
}

void D_RegisterLoopCallbacks(data_t* data, loop_interface_t *i)
{
	data->d_loop.loop_interface = i;
}
