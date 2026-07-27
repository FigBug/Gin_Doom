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
//	WAD I/O functions.
//


#ifndef __W_WAD__
#define __W_WAD__

#include <stdio.h>

#include "doomtype.h"
#include "d_mode.h"

#include "w_file.h"


//
// TYPES
//

//
// WADFILE I/O related stuff.
//

typedef struct lumpinfo_s lumpinfo_t;

struct lumpinfo_s
{
    char	name[8];
    wad_file_t *wad_file;
    int		position;
    int		size;
    void       *cache;

    // Used for hash table lookups

    lumpinfo_t *next;
};


// The lump directory (lumpinfo/numlumps) and hash table now live in data_t
// so each Doom instance owns its own WADs.

int	W_NumLumps (data_t* data);

wad_file_t *W_AddFile (data_t* data, char *filename);

int	W_CheckNumForName (data_t* data, char* name);
int	W_GetNumForName (data_t* data, char* name);

int	W_LumpLength (data_t* data, unsigned int lump);
void    W_ReadLump (data_t* data, unsigned int lump, void *dest);

void*	W_CacheLumpNum (data_t* data, int lump, int tag);
void*	W_CacheLumpName (data_t* data, char* name, int tag);

void    W_GenerateHashTable(data_t* data);

extern unsigned int W_LumpNameHash(const char *s);

void    W_ReleaseLumpNum(data_t* data, int lump);
void    W_ReleaseLumpName(data_t* data, char *name);

void W_CheckCorrectIWAD(data_t* data, GameMission_t mission);

#endif
