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
//	Savegame I/O, archiving, persistence.
//


#ifndef __P_SAVEG__
#define __P_SAVEG__

#include <stdio.h>

// maximum size of a savegame description

#define SAVESTRINGSIZE 24

// temporary filename to use while saving.

char *P_TempSaveGameFile(data_t* data);

// filename to use for a savegame slot

char *P_SaveGameFile(data_t* data, int slot);

// Savegame file header read/write functions

boolean P_ReadSaveGameHeader(data_t* data);
void P_WriteSaveGameHeader(data_t* data, char *description);

// Savegame end-of-file read/write functions

boolean P_ReadSaveGameEOF(data_t* data);
void P_WriteSaveGameEOF(data_t* data);

// Persistent storage/archiving.
// These are the load / save game routines.
void P_ArchivePlayers (data_t* data);
void P_UnArchivePlayers (data_t* data);
void P_ArchiveWorld (data_t* data);
void P_UnArchiveWorld (data_t* data);
void P_ArchiveThinkers (data_t* data);
void P_UnArchiveThinkers (data_t* data);
void P_ArchiveSpecials (data_t* data);
void P_UnArchiveSpecials (data_t* data);

extern FILE *save_stream;
extern boolean savegame_error;


#endif
