//
// CouchDoom basic bot. Fills the ticcmd for an AI-controlled player.
//
// A bot runs only on its OWN instance, for its own player (couch_index); the
// arbiter distributes the resulting ticcmd to every instance like any human's.
// It therefore reads simulation state (positions, line of sight) but writes
// ONLY the ticcmd and its own private RNG (data->bot_seed) - never the game
// RNG (rndindex) or any other synced state - so it cannot desync the lockstep.
//

#ifndef __BOT__
#define __BOT__

#include "doomtype.h"
#include "d_ticcmd.h"

typedef struct data_s data_t;

// Overwrite the movement/button fields of cmd with a bot's decision (leaves
// consistancy intact). Called after BuildTiccmd for a bot player.
void Bot_BuildTiccmd (data_t* data, ticcmd_t* cmd);

#endif
