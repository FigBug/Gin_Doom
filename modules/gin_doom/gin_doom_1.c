/*==============================================================================

 Copyright 2023 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

//==============================================================================

#if defined(__clang__)
 #pragma clang diagnostic ignored "-Wstrict-prototypes"
 #pragma clang diagnostic ignored "-Wmissing-prototypes"
 #pragma clang diagnostic ignored "-Wimplicit-int-conversion"
 #pragma clang diagnostic ignored "-Wsign-conversion"
 #pragma clang diagnostic ignored "-Wunused-parameter"
 #pragma clang diagnostic ignored "-Wsign-compare"
 #pragma clang diagnostic ignored "-Wshadow"
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
 #pragma clang diagnostic ignored "-Wshift-sign-overflow"
 #pragma clang diagnostic ignored "-Wswitch-enum"
 #pragma clang diagnostic ignored "-Wpointer-to-int-cast"
 #pragma clang diagnostic ignored "-Wcast-align"
 #pragma clang diagnostic ignored "-Wunused-but-set-variable"
 #pragma clang diagnostic ignored "-Wabsolute-value"
 #pragma clang diagnostic ignored "-Wunknown-warning-option"
 #pragma clang diagnostic ignored "-Wmisleading-indentation"
 #pragma clang diagnostic ignored "-Wstatic-in-inline"
 #pragma clang diagnostic ignored "-Wpedantic"
 #pragma clang diagnostic ignored "-Wformat-pedantic"
 #pragma clang diagnostic ignored "-Wc23-extensions"
 #pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#elif defined(_MSC_VER)
 // MSVC equivalents of the clang ignores above: implicit conversions (4244,
 // 4245, 4267, 4311), sign compare (4018, 4389), unreferenced params (4100),
 // old-style prototypes / function pointer mismatches (4113, 4152), shadowing
 // (4456, 4459), plus CRT deprecation (4996) and other C89-era noise.
 #pragma warning(disable: 4018 4100 4113 4146 4152 4210 4244 4245 4267 4295 4310 4311 4389 4456 4459 4996)
#endif

#define FEATURE_SOUND

#include "source/doomgeneric/am_map.c"
#include "source/doomgeneric/d_event.c"
#include "source/doomgeneric/d_items.c"
#include "source/doomgeneric/d_iwad.c"
#include "source/doomgeneric/d_loop.c"
#include "source/doomgeneric/d_main.c"
#include "source/doomgeneric/d_mode.c"
#include "source/doomgeneric/d_net.c"
#include "source/doomgeneric/doomdef.c"
#include "source/doomgeneric/doomgeneric.c"
#include "source/doomgeneric/doomstat.c"
#include "source/doomgeneric/dstrings.c"
#include "source/doomgeneric/dummy.c"
#include "source/doomgeneric/f_finale.c"
#include "source/doomgeneric/f_wipe.c"
#include "source/doomgeneric/g_game.c"
#include "source/doomgeneric/gusconf.c"
#include "source/doomgeneric/hu_lib.c"
#include "source/doomgeneric/hu_stuff.c"
#include "source/doomgeneric/i_cdmus.c"
#include "source/doomgeneric/i_endoom.c"
#include "source/doomgeneric/i_input.c"
#include "source/doomgeneric/i_joystick.c"
#include "source/doomgeneric/i_scale.c"
#include "source/doomgeneric/i_sound.c"
#include "source/doomgeneric/i_system.c"
#include "source/doomgeneric/i_timer.c"
#include "source/doomgeneric/icon.c"
#include "source/doomgeneric/info.c"
#include "source/doomgeneric/m_argv.c"
#include "source/doomgeneric/m_bbox.c"
#include "source/doomgeneric/m_cheat.c"
#include "source/doomgeneric/m_config.c"
#include "source/doomgeneric/m_controls.c"
#include "source/doomgeneric/m_fixed.c"
#include "source/doomgeneric/m_menu.c"
#include "source/doomgeneric/m_misc.c"
#include "source/doomgeneric/m_random.c"
#include "source/doomgeneric/memio.c"
#include "source/doomgeneric/mus2mid.c"
#include "source/doomgeneric/p_ceilng.c"
#include "source/doomgeneric/p_doors.c"

