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

#include "source/doomgeneric/wi_stuff.c"
#include "source/doomgeneric/i_video.c"
#include "source/doomgeneric/i_sdlsound.c"
// NOTE: i_sdlmusic.c is no longer built. The music module (DG_music_module)
// is now provided by the OPL FM-synth implementation in i_oplmusic.c, which
// is compiled via gin_doom_4.c .. gin_doom_7.c below.
