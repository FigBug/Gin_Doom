#ifndef DATA
#define DATA

typedef struct data_s data_t;

#include "d_mode.h"
#include "d_event.h"
#include "doomdef.h"

struct data_s
{
	void*		user_data;

	uint32_t* 	DG_ScreenBuffer;

	// m_argv.c
	int				myargc;
	char**			myargv;

	// d_main.c
	char *          savegamedir;

	// location of IWAD and WAD files
	char *          iwadfile;

	boolean		    devparm;	// started game with -devparm
	boolean         nomonsters;	// checkparm of -nomonsters
	boolean         respawnparm;	// checkparm of -respawn
	boolean         fastparm;	// checkparm of -fast
	uint32_t        runloop;    // run the event loop

	skill_t			startskill;
	int             startepisode;
	int				startmap;
	boolean			autostart;
	int             startloadgame;

	boolean			advancedemo;

	// Store demo, do not accept any inputs
	boolean         storedemo;

	// "BFG Edition" version of doom2.wad does not include TITLEPIC.
	boolean         bfgedition;

	// If true, the main game loop has started.
	boolean         main_loop_started;

	char			wadfile[1024];		// primary wad file
	char			mapdir[1024];           // directory of development maps

	int             show_endoom;

	// m_random.c
	int             rndindex;
	int             prndindex;

	// f_wipe.c
	boolean         wipe_go;
	byte*           wipe_scr_start;
	byte*           wipe_scr_end;
	byte*           wipe_scr;
	int*            wipe_y;

	// d_event.c
	event_t         d_events[64];
	int             d_eventhead;
	int             d_eventtail;

	// core game state (doomstat.h / g_game.c / d_loop.c / p_tick.c)
	int             leveltime;
	int             gametic;
	gamestate_t     gamestate;
	skill_t         gameskill;
	boolean         respawnmonsters;
	int             gameepisode;
	int             gamemap;
	int             timelimit;
	boolean         paused;
	boolean         usergame;
	boolean         nodrawers;
	boolean         viewactive;
	int             deathmatch;
	boolean         netgame;
	int             consoleplayer;
	int             displayplayer;
	int             levelstarttic;
	int             totalkills;
	int             totalitems;
	int             totalsecret;
	boolean         demorecording;
	boolean         demoplayback;
	boolean         singledemo;
	boolean         precache;


};

#endif //DOOM_GENERIC
