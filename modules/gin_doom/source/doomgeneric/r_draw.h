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
//	System specific interface stuff.
//


#ifndef __R_DRAW__
#define __R_DRAW__





// first pixel in a column


// The span blitting interface.
// Hook in assembler or system specific BLT
//  here.
void	R_DrawColumn (data_t* data);
void	R_DrawColumnLow (data_t* data);

// The Spectre/Invisibility effect.
void	R_DrawFuzzColumn (data_t* data);
void	R_DrawFuzzColumnLow (data_t* data);

// Draw with color translation tables,
//  for player sprite rendering,
//  Green/Red/Blue/Indigo shirts.
void	R_DrawTranslatedColumn (data_t* data);
void	R_DrawTranslatedColumnLow (data_t* data);

void
R_VideoErase
( data_t* data,
  unsigned	ofs,
  int		count );




// start of a 64*64 tile image



// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
void	R_DrawSpan (data_t* data);

// Low resolution mode, 160x200?
void	R_DrawSpanLow (data_t* data);


void
R_InitBuffer
( data_t* data,
  int		width,
  int		height );


// Initialize color translation tables,
//  for player rendering etc.
void	R_InitTranslationTables (data_t* data);



// Rendering function.
void	R_FillBackScreen (data_t* data);

// If the view size is not full screen, draws a border around it.
void	R_DrawViewBorder (data_t* data);



#endif
