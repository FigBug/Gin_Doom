//
// CouchDoom basic bot - see bot.h. Wanders, and shoots whatever shootable
// thing it can currently see (other players and monsters).
//

#include "doomdef.h"
#include "d_event.h"      // BT_ATTACK, BT_USE
#include "d_ticcmd.h"
#include "p_local.h"      // P_CheckSight, P_AproxDistance, P_MobjThinker
#include "r_main.h"       // R_PointToAngle2
#include "tables.h"       // ANG* / angle_t
#include "data.h"

#include "bot.h"

#define BOT_FORWARD    0x32              // Doom "run" forwardmove
#define BOT_SIDE       0x28              // Doom "run" sidemove
#define BOT_RANGE      (2048 * FRACUNIT) // ignore targets farther than this
#define BOT_TURN_MAX   1800              // max angleturn units per tic (~10 deg)
#define BOT_FIRE_TOL   1400              // fire when facing within ~7.7 deg

// Private RNG (LCG), independent of the game's P_Random/rndindex so a bot's
// decisions never perturb the synced simulation.
static int Bot_Rand (data_t* data)
{
    data->bot_seed = data->bot_seed * 1103515245u + 12345u;
    return (int) ((data->bot_seed >> 16) & 0xff);   // 0..255
}

// Nearest shootable, currently-visible thing (not ourselves).
static mobj_t* Bot_FindTarget (data_t* data, mobj_t* me)
{
    thinker_t* th;
    mobj_t*    best = NULL;
    fixed_t    bestdist = BOT_RANGE;

    for (th = data->thinkercap.next; th != &data->thinkercap; th = th->next)
    {
        mobj_t* m;
        fixed_t dist;

        if (th->function.acp1 != (actionf_p1) P_MobjThinker)
            continue;

        m = (mobj_t*) th;
        if (m == me || m->health <= 0 || !(m->flags & MF_SHOOTABLE))
            continue;

        dist = P_AproxDistance (m->x - me->x, m->y - me->y);
        if (dist >= bestdist)
            continue;
        if (!P_CheckSight (data, me, m))
            continue;

        best = m;
        bestdist = dist;
    }

    return best;
}

void Bot_BuildTiccmd (data_t* data, ticcmd_t* cmd)
{
    int       me = data->couch_index;
    player_t* pl = &data->players[me];
    mobj_t*   mo = pl->mo;
    mobj_t*   target;

    if (mo == NULL || pl->playerstate == PST_DEAD)
    {
        // Dead: pressing attack requests a deathmatch respawn.
        cmd->buttons |= BT_ATTACK;
        return;
    }

    target = Bot_FindTarget (data, mo);

    if (target != NULL)
    {
        angle_t want = R_PointToAngle2 (data, mo->x, mo->y, target->x, target->y);
        int     turn = (int) ((angle_t) (want - mo->angle) >> 16);  // 0..65535
        if (turn >= 32768)
            turn -= 65536;                                          // shortest way, signed

        if (turn >  BOT_TURN_MAX) turn =  BOT_TURN_MAX;
        if (turn < -BOT_TURN_MAX) turn = -BOT_TURN_MAX;
        cmd->angleturn = (short) (cmd->angleturn + turn);

        if (turn > -BOT_FIRE_TOL && turn < BOT_FIRE_TOL)
            cmd->buttons |= BT_ATTACK;

        // Keep moving so bots aren't static targets.
        cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);
        if (Bot_Rand (data) < 40)
            cmd->sidemove = (signed char) (cmd->sidemove + (Bot_Rand (data) < 128 ? BOT_SIDE : -BOT_SIDE));
    }
    else
    {
        // Wander: walk forward, occasionally turn or strafe.
        cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);

        if (Bot_Rand (data) < 24)
            cmd->angleturn = (short) (cmd->angleturn + (Bot_Rand (data) - 128) * 16);
        if (Bot_Rand (data) < 16)
            cmd->sidemove = (signed char) (cmd->sidemove + (Bot_Rand (data) < 128 ? BOT_SIDE : -BOT_SIDE));
    }
}
