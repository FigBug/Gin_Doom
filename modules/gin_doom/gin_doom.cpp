/*==============================================================================

 Copyright 2023 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

//==============================================================================

#include "gin_doom.h"

//==============================================================================

#pragma clang diagnostic ignored "-Wmissing-prototypes"

#define FEATURE_SOUND

extern "C"
{
#include "source/doomgeneric/data.h"
#include "source/doomgeneric/m_argv.h"
#include "source/doomgeneric/doomkeys.h"
#include "source/doomgeneric/i_sound.h"
#include "source/doomgeneric/w_wad.h"
#include "source/doomgeneric/z_zone.h"
#include "source/doomgeneric/deh_str.h"
#include "source/doomgeneric/m_misc.h"
}

namespace gin
{
#include "source/gin_doom.cpp"
#include "source/gin_doomcomponent.cpp"
#include "source/gin_doomaudioengine.cpp"
}
