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
//   Duh.
// 


#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"
#include "d_ticcmd.h"


//
// GAME
//
void G_DeathMatchSpawnPlayer (data_t* data, int playernum);

void G_InitNew (data_t* data, skill_t skill, int episode, int map);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (skill_t skill, int episode, int map);

void G_DeferedPlayDemo (char* demo);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame (char* name);

void G_DoLoadGame (data_t* data);

// Called by M_Responder.
void G_SaveGame (int slot, char* description);

// Only called by startup code.
void G_RecordDemo (data_t* data, char* name);

void G_BeginRecording (data_t* data);

void G_PlayDemo (char* name);
void G_TimeDemo (data_t* data, char* name);
boolean G_CheckDemoStatus (data_t* data);

void G_ExitLevel (void);
void G_SecretExitLevel (void);

void G_WorldDone (void);

// Read current data from inputs and build a player movement command.

void G_BuildTiccmd (data_t* data, ticcmd_t *cmd, int maketic);

void G_Ticker (data_t* data);
boolean G_Responder (event_t*	ev);

void G_ScreenShot (void);

void G_DrawMouseSpeedBox(void);
int G_VanillaVersionCode(data_t* data);

extern int vanilla_savegame_limit;
extern int vanilla_demo_limit;
#endif

