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
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//


#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"

// Needed because we are refering to patches.
#include "v_patch.h"

//
// VIDEO
//

#define CENTERY			(SCREENHEIGHT/2)


extern int dirtybox[4];

extern byte *tinttable;

// haleyjd 08/28/10: implemented for Strife support
// haleyjd 08/28/10: Patch clipping callback, implemented to support Choco
// Strife.
typedef boolean (*vpatchclipfunc_t)(patch_t *, int, int);
void V_SetPatchClipCallback(vpatchclipfunc_t func);


// Allocates buffer screens, call before R_Init.
void V_Init (data_t* data);

// Draw a block from the specified source screen to the screen.

void V_CopyRect(data_t* data, int srcx, int srcy, byte *source,
                int width, int height,
                int destx, int desty);

void V_DrawPatch(data_t* data, int x, int y, patch_t *patch);
void V_DrawPatchFlipped(data_t* data, int x, int y, patch_t *patch);
void V_DrawTLPatch(data_t* data, int x, int y, patch_t *patch);
void V_DrawAltTLPatch(data_t* data, int x, int y, patch_t * patch);
void V_DrawShadowedPatch(data_t* data, int x, int y, patch_t *patch);
void V_DrawXlaPatch(data_t* data, int x, int y, patch_t * patch);     // villsa [STRIFE]
void V_DrawPatchDirect(data_t* data, int x, int y, patch_t *patch);

// Draw a linear block of pixels into the view buffer.

void V_DrawBlock(data_t* data, int x, int y, int width, int height, byte *src);

void V_MarkRect(data_t* data, int x, int y, int width, int height);

void V_DrawFilledBox(data_t* data, int x, int y, int w, int h, int c);
void V_DrawHorizLine(data_t* data, int x, int y, int w, int c);
void V_DrawVertLine(data_t* data, int x, int y, int h, int c);
void V_DrawBox(data_t* data, int x, int y, int w, int h, int c);

// Draw a raw screen lump

void V_DrawRawScreen(data_t* data, byte *raw);

// Temporarily switch to using a different buffer to draw graphics, etc.

void V_UseBuffer(data_t* data, byte *buffer);

// Return to using the normal screen buffer to draw graphics.

void V_RestoreBuffer(data_t* data);

// Save a screenshot of the current screen to a file, named in the 
// format described in the string passed to the function, eg.
// "DOOM%02i.pcx"

void V_ScreenShot(data_t* data, char *format);

// Load the lookup table for translucency calculations from the TINTTAB
// lump.

void V_LoadTintTable(void);

// villsa [STRIFE]
// Load the lookup table for translucency calculations from the XLATAB
// lump.

void V_LoadXlaTable(void);

void V_DrawMouseSpeedBox(data_t* data, int speed);

#endif

