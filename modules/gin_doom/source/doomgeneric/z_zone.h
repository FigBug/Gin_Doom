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
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//	Remark: this was the only stuff that, according
//	 to John Carmack, might have been useful for
//	 Quake.
//



#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>
#include "data.h"

//
// ZONE MEMORY
// PU - purge tags.

enum
{
    PU_STATIC = 1,                  // static entire execution time
    PU_SOUND,                       // static while playing
    PU_MUSIC,                       // static while playing
    PU_FREE,                        // a free block
    PU_LEVEL,                       // static until level exited
    PU_LEVSPEC,                     // a special thinker in a level
    
    // Tags >= PU_PURGELEVEL are purgable whenever needed.

    PU_PURGELEVEL,
    PU_CACHE,

    // Total number of different tag types

    PU_NUM_TAGS
};
        

void	Z_Init (data_t* data);
void*	Z_Malloc (data_t* data, int size, int tag, void *ptr);
void    Z_Free (data_t* data, void *ptr);
void    Z_FreeTags (data_t* data, int lowtag, int hightag);
void    Z_DumpHeap (data_t* data, int lowtag, int hightag);
void    Z_FileDumpHeap (data_t* data, FILE *f);
void    Z_CheckHeap (data_t* data);
void    Z_ChangeTag2 (void *ptr, int tag, char *file, int line);
void    Z_ChangeUser(void *ptr, void **user);
int     Z_FreeMemory (data_t* data);
unsigned int Z_ZoneSize(data_t* data);

//
// This is used to get the local FILE:LINE info from CPP
// prior to really call the function in question.
//
#define Z_ChangeTag(p,t)                                       \
    Z_ChangeTag2((p), (t), __FILE__, __LINE__)


#endif
