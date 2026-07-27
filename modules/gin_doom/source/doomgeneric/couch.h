//
// CouchDoom fake-network lockstep arbiter.
//
// Several Doom instances (each its own data_t, own thread) are kept in perfect
// lockstep without any real networking: every tic, each instance builds its
// local ticcmd, then all instances meet at a cyclic barrier where the full set
// of commands is distributed to everyone via D_ReceiveTic. This is Doom's own
// peer-to-peer deathmatch model with the transport replaced by a function call.
//
// Inactive (a no-op) unless two or more instances register, so a normal
// single-instance build is unaffected.
//

#ifndef __COUCH__
#define __COUCH__

#include "doomtype.h"

typedef struct data_s data_t;

// Register an instance as player `index` of a `numplayers`-way game. Called
// once per instance before its game loop starts.
void Couch_Register(data_t* data, int index, int numplayers);

// True once a multi-player couch game has been registered.
boolean Couch_Active(void);

// Called once per tic by each instance, after it has written its local ticcmd
// into ticdata[maketic]. Blocks until all instances have arrived, distributes
// everyone's commands (D_ReceiveTic), paces the group to TICRATE, and releases.
void Couch_Barrier(data_t* data);

// Wake any instances waiting at the barrier (used on shutdown so a departing
// instance doesn't deadlock the others).
void Couch_Shutdown(void);

// Clear all arbiter state for a fresh session (host calls before a new match).
void Couch_Reset(void);

// A player chose Quit; the host polls this and returns everyone to the menu.
void Couch_RequestQuit(void);
boolean Couch_QuitRequested(void);

#endif
