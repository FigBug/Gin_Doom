//
// CouchDoom basic bot - see bot.h. Wanders, prefers to fight other players,
// keeps its distance with a ranged weapon (charging in only with fists), juke-
// strafes while engaging, and steps off / avoids damaging "poison" floors.
//

#include "doomdef.h"
#include "d_event.h"      // BT_ATTACK, BT_USE
#include "d_ticcmd.h"
#include "m_fixed.h"      // FixedMul
#include "p_local.h"      // P_CheckSight, P_AproxDistance, P_MobjThinker
#include "r_main.h"       // R_PointToAngle2, R_PointInSubsector
#include "tables.h"       // ANG*, finesine/finecosine, ANGLETOFINESHIFT
#include "data.h"

#include "bot.h"

#define BOT_FORWARD    0x32              // Doom "run" forwardmove
#define BOT_SIDE       0x28              // Doom "run" sidemove
#define BOT_RANGE      (2048 * FRACUNIT) // ignore targets farther than this
#define BOT_TURN_MAX   1800              // max angleturn units per tic (~10 deg)
#define BOT_FIRE_TOL   1400              // fire when facing within ~7.7 deg
#define BOT_NEAR       (256 * FRACUNIT)  // back off if a ranged target is closer
#define BOT_FAR        (640 * FRACUNIT)  // close in if a ranged target is farther
#define BOT_AHEAD      (64 * FRACUNIT)   // look-ahead distance for floor hazards

// Private RNG (LCG), independent of the game's P_Random/rndindex so a bot's
// decisions never perturb the synced simulation.
static int Bot_Rand (data_t* data)
{
    data->bot_seed = data->bot_seed * 1103515245u + 12345u;
    return (int) ((data->bot_seed >> 16) & 0xff);   // 0..255
}

// True if the floor at (x,y) hurts (nukage/slime/lava/end-level sectors).
static boolean Bot_FloorHurts (data_t* data, fixed_t x, fixed_t y)
{
    int s = R_PointInSubsector (data, x, y)->sector->special;
    return s == 4 || s == 5 || s == 7 || s == 11 || s == 16;
}

// Nearest shootable, currently-visible thing. Players are preferred over
// monsters (a visible player is chosen even if a monster is closer).
static mobj_t* Bot_FindTarget (data_t* data, mobj_t* me)
{
    thinker_t* th;
    mobj_t*    bestPlayer = NULL;  fixed_t bestPlayerDist = BOT_RANGE;
    mobj_t*    bestMonster = NULL; fixed_t bestMonsterDist = BOT_RANGE;

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

        if (m->player != NULL)
        {
            if (dist < bestPlayerDist && P_CheckSight (data, me, m))
            { bestPlayer = m; bestPlayerDist = dist; }
        }
        else
        {
            if (dist < bestMonsterDist && P_CheckSight (data, me, m))
            { bestMonster = m; bestMonsterDist = dist; }
        }
    }

    return bestPlayer != NULL ? bestPlayer : bestMonster;
}

void Bot_BuildTiccmd (data_t* data, ticcmd_t* cmd)
{
    int       me = data->couch_index;
    player_t* pl = &data->players[me];
    mobj_t*   mo = pl->mo;
    mobj_t*   target;
    fixed_t   moved, aheadx, aheady;
    boolean   onHurt, aheadHurt;

    // Only act during play - not on the intermission (frag table).
    if (data->gamestate != GS_LEVEL)
        return;

    if (mo == NULL || pl->playerstate == PST_DEAD)
    {
        // Dead: P_DeathThink respawns on BT_USE.
        cmd->buttons |= BT_USE;
        return;
    }

    // Periodically tap Use (a rising edge every 16 tics) so bots can open doors
    // and hit switches they walk into. Harmless where there's nothing to use.
    if ((data->leveltime & 15) < 2)
        cmd->buttons |= BT_USE;

    // How far did we actually move since last tic? (We always command forward
    // motion, so little movement means we're jammed on geometry.)
    moved = P_AproxDistance (mo->x - data->bot_lastx, mo->y - data->bot_lasty);
    data->bot_lastx = mo->x;
    data->bot_lasty = mo->y;
    if (moved < 2 * FRACUNIT)
        data->bot_stuck++;
    else
        data->bot_stuck = 0;

    // Floor hazard checks: where we stand, and a short step ahead.
    aheadx = mo->x + FixedMul (BOT_AHEAD, finecosine[mo->angle >> ANGLETOFINESHIFT]);
    aheady = mo->y + FixedMul (BOT_AHEAD, finesine  [mo->angle >> ANGLETOFINESHIFT]);
    onHurt    = Bot_FloorHurts (data, mo->x, mo->y);
    aheadHurt = Bot_FloorHurts (data, aheadx, aheady);

    // Escape: keep turning one way and pushing forward until free (used to get
    // unstuck, and to get off a damaging floor).
    if (data->bot_escape > 0)
    {
        data->bot_escape--;
        cmd->angleturn = (short) (cmd->angleturn + data->bot_escape_turn);
        cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);
        return;
    }
    if (data->bot_stuck >= 8 || onHurt)
    {
        data->bot_escape = 12;
        data->bot_escape_turn = (Bot_Rand (data) < 128) ? BOT_TURN_MAX : -BOT_TURN_MAX;
        data->bot_stuck = 0;
        cmd->angleturn = (short) (cmd->angleturn + data->bot_escape_turn);
        cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);
        return;
    }

    // Occasionally flip the juke direction.
    if (data->bot_strafe == 0 || Bot_Rand (data) < 8)
        data->bot_strafe = (Bot_Rand (data) < 128) ? 1 : -1;

    target = Bot_FindTarget (data, mo);

    if (target != NULL)
    {
        angle_t  want = R_PointToAngle2 (data, mo->x, mo->y, target->x, target->y);
        int      turn = (int) ((angle_t) (want - mo->angle) >> 16);
        fixed_t  dist = P_AproxDistance (target->x - mo->x, target->y - mo->y);
        weapontype_t w = pl->readyweapon;
        boolean  melee = (w == wp_fist || w == wp_chainsaw);

        if (turn >= 32768)
            turn -= 65536;
        if (turn >  BOT_TURN_MAX) turn =  BOT_TURN_MAX;
        if (turn < -BOT_TURN_MAX) turn = -BOT_TURN_MAX;
        cmd->angleturn = (short) (cmd->angleturn + turn);

        if (turn > -BOT_FIRE_TOL && turn < BOT_FIRE_TOL)
            cmd->buttons |= BT_ATTACK;

        if (melee)
        {
            // Only with fists/chainsaw do we charge to melee range.
            if (!aheadHurt)
                cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);
        }
        else
        {
            // Ranged: hold a mid distance and juke-strafe rather than charging.
            if (dist < BOT_NEAR)
                cmd->forwardmove = (signed char) (cmd->forwardmove - BOT_FORWARD);
            else if (dist > BOT_FAR && !aheadHurt)
                cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);

            cmd->sidemove = (signed char) (cmd->sidemove
                              + (data->bot_strafe > 0 ? BOT_SIDE : -BOT_SIDE));
        }
    }
    else
    {
        // Wander: walk forward, but turn away from a hazard ahead.
        if (aheadHurt)
            cmd->angleturn = (short) (cmd->angleturn + (Bot_Rand (data) - 128) * 24);
        else
            cmd->forwardmove = (signed char) (cmd->forwardmove + BOT_FORWARD);

        if (Bot_Rand (data) < 24)
            cmd->angleturn = (short) (cmd->angleturn + (Bot_Rand (data) - 128) * 16);
        if (Bot_Rand (data) < 16)
            cmd->sidemove = (signed char) (cmd->sidemove + (Bot_Rand (data) < 128 ? BOT_SIDE : -BOT_SIDE));
    }
}
