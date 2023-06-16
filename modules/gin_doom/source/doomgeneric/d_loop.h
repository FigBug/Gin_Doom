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
//	Main loop stuff.
//

#ifndef __D_LOOP__
#define __D_LOOP__

#include "net_defs.h"
#include "data.h"

// Register callback functions for the main loop code to use.
void D_RegisterLoopCallbacks(data_t* data, loop_interface_t *i);

// Create any new ticcmds and broadcast to other players.
void NetUpdate (data_t* data);

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame (data_t* data);

//? how many ticks to run?
void TryRunTics (data_t* data);

// Called at start of game loop to initialize timers
void D_StartGameLoop (data_t* data);

// Initialize networking code and connect to server.

boolean D_InitNetGame(data_t* data, net_connect_data_t *connect_data);

// Start game with specified settings. The structure will be updated
// with the actual settings for the game.

void D_StartNetGame(data_t* data, net_gamesettings_t *settings,
                    netgame_startup_callback_t callback);

#endif

