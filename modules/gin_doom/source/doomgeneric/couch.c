//
// CouchDoom fake-network lockstep arbiter. See couch.h.
//

#include <string.h>
#include <stdint.h>

#include "doomtype.h"
#include "doomdef.h"      // MAXPLAYERS
#include "net_defs.h"     // NET_MAXPLAYERS, BACKUPTICS
#include "d_ticcmd.h"     // ticcmd_t
#include "data.h"         // data_t (ticdata, maketic, recvtic, gametic, ...)
#include "d_loop.h"       // D_ReceiveTic
#include "i_timer.h"      // TICRATE, I_Sleep, I_GetTimeMS

#include "couch.h"

// ---------------------------------------------------------------------------
// Cross-platform mutex + condition variable (statically initialised).
// ---------------------------------------------------------------------------

#ifdef _WIN32
  #include <windows.h>
  static SRWLOCK            couch_mtx = SRWLOCK_INIT;
  static CONDITION_VARIABLE couch_cnd = CONDITION_VARIABLE_INIT;
  #define C_LOCK()   AcquireSRWLockExclusive(&couch_mtx)
  #define C_UNLOCK() ReleaseSRWLockExclusive(&couch_mtx)
  #define C_WAIT()   SleepConditionVariableSRW(&couch_cnd, &couch_mtx, INFINITE, 0)
  #define C_BCAST()  WakeAllConditionVariable(&couch_cnd)
#else
  #include <pthread.h>
  static pthread_mutex_t couch_mtx = PTHREAD_MUTEX_INITIALIZER;
  static pthread_cond_t  couch_cnd = PTHREAD_COND_INITIALIZER;
  #define C_LOCK()   pthread_mutex_lock(&couch_mtx)
  #define C_UNLOCK() pthread_mutex_unlock(&couch_mtx)
  #define C_WAIT()   pthread_cond_wait(&couch_cnd, &couch_mtx)
  #define C_BCAST()  pthread_cond_broadcast(&couch_cnd)
#endif

// ---------------------------------------------------------------------------
// State (inter-instance, so deliberately global — this IS the shared arbiter).
// ---------------------------------------------------------------------------

static data_t*      couch_data[MAXPLAYERS];
static int          couch_n = 0;            // number of players (0/1 = inactive)
static int          couch_arrived = 0;      // arrivals at the current barrier
static unsigned int couch_gen = 0;          // barrier generation
static int          couch_tic = 0;          // next tic to distribute
static int          couch_start_ms = -1;    // wall-clock of tic 0, for pacing
static int          couch_abort = 0;

void Couch_Register(data_t* data, int index, int numplayers)
{
    C_LOCK();

    if (index >= 0 && index < MAXPLAYERS)
    {
        couch_data[index] = data;

        if (numplayers > couch_n)
        {
            couch_n = numplayers;
        }
    }

    C_UNLOCK();
}

boolean Couch_Active(void)
{
    return couch_n > 1;
}

void Couch_Shutdown(void)
{
    C_LOCK();
    couch_abort = 1;
    C_BCAST();
    C_UNLOCK();
}

void Couch_Barrier(data_t* data)
{
    unsigned int gen;

    C_LOCK();

    if (couch_abort)
    {
        C_UNLOCK();
        return;
    }

    gen = couch_gen;
    ++couch_arrived;

    if (couch_arrived >= couch_n)
    {
        // Last to arrive: every instance has built its local command for
        // couch_tic. Assemble the full set and hand it to all of them.
        int      T = couch_tic;
        int      k;
        ticcmd_t cmds[NET_MAXPLAYERS];
        boolean  mask[NET_MAXPLAYERS];

        memset(cmds, 0, sizeof(cmds));
        memset(mask, 0, sizeof(mask));

        for (k = 0; k < couch_n; ++k)
        {
            cmds[k] = couch_data[k]->ticdata[T % BACKUPTICS].cmds[k];
            mask[k] = true;
        }

        // recvtic == couch_tic for every instance, so D_ReceiveTic writes into
        // the same slot the local command lives in, then advances recvtic.
        for (k = 0; k < couch_n; ++k)
        {
            D_ReceiveTic(couch_data[k], cmds, mask);
        }

        couch_tic = T + 1;

        // Pace the whole group to TICRATE (absolute schedule, no drift creep).
        if (couch_start_ms < 0)
        {
            couch_start_ms = I_GetTimeMS(data);
        }
        {
            int target = couch_start_ms + (T + 1) * (1000 / TICRATE);
            int now    = I_GetTimeMS(data);
            if (target > now)
            {
                I_Sleep(data, target - now);
            }
        }

        couch_arrived = 0;
        ++couch_gen;
        C_BCAST();
    }
    else
    {
        while (couch_gen == gen && !couch_abort)
        {
            C_WAIT();
        }
    }

    C_UNLOCK();
}
