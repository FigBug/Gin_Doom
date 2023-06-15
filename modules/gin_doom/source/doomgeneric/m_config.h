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
//      Configuration file interface.
//    


#ifndef __M_CONFIG__
#define __M_CONFIG__

#include "doomtype.h"

void M_LoadDefaults(data_t* data);
void M_SaveDefaults(data_t* data);
void M_SaveDefaultsAlternate(data_t* data, char *main, char *extra);
void M_SetConfigDir(char *dir);
void M_BindVariable(char *name, void *variable);
boolean M_SetVariable(char *name, char *value);
int M_GetIntVariable(char *name);
const char *M_GetStrVariable(char *name);
float M_GetFloatVariable(char *name);
void M_SetConfigFilenames(char *main_config, char *extra_config);
char *M_GetSaveGameDir(char *iwadname);

extern char *configdir;

#endif
