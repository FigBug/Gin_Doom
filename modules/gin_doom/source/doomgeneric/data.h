#ifndef DATA
#define DATA

#define TICRATE 35

#include "d_mode.h"
#include "m_fixed.h"
#include "d_player.h"
#include "v_patch.h"
#include "m_cheat.h"
#include "d_event.h"

//
// am_map
//
#define AM_NUMMARKPOINTS 10

typedef struct
{
	fixed_t		x,y;
} mpoint_t;

typedef struct
{
	int 	cheating;
	int 	grid;

	int 	leveljuststarted; 	// kluge until AM_LevelInit() is called

	boolean    	automapactive;
	int 	finit_width;
	int 	finit_height;

	// location of window on screen
	int 	f_x;
	int	f_y;

	// size of window on screen
	int 	f_w;
	int	f_h;

	int 	lightlev; 		// used for funky strobing effect
	byte*	fb; 			// pseudo-frame buffer
	int 	amclock;

	mpoint_t m_paninc; // how far the window pans each tic (map coords)
	fixed_t 	mtof_zoommul; // how far the window zooms in each tic (map coords)
	fixed_t 	ftom_zoommul; // how far the window zooms in each tic (fb coords)

	fixed_t 	m_x, m_y;   // LL x,y where the window is on the map (map coords)
	fixed_t 	m_x2, m_y2; // UR x,y where the window is on the map (map coords)

	//
	// width/height of window on map (map coords)
	//
	fixed_t 	m_w;
	fixed_t	m_h;

	// based on level size
	fixed_t 	min_x;
	fixed_t	min_y;
	fixed_t 	max_x;
	fixed_t  max_y;

	fixed_t 	max_w; // max_x-min_x,
	fixed_t  max_h; // max_y-min_y

	// based on player size
	fixed_t 	min_w;
	fixed_t  min_h;

	fixed_t 	min_scale_mtof; // used to tell when to stop zooming out
	fixed_t 	max_scale_mtof; // used to tell when to stop zooming in

	// old stuff for recovery later
	fixed_t old_m_w, old_m_h;
	fixed_t old_m_x, old_m_y;

	// old location used by the Follower routine
	mpoint_t f_oldloc;

	// used by MTOF to scale from map-to-frame-buffer coords
	fixed_t scale_mtof;
	// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
	fixed_t scale_ftom;

	player_t *plr; // the player represented by an arrow

	patch_t *marknums[10]; // numbers used for marking by the automap
	mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
	int markpointnum; // next point to be assigned
	int followplayer; // specifies whether to follow the player around

	cheatseq_t cheat_amap;

	boolean stopped;

} am_map_t;

//
// d_event
//
#define MAXEVENTS 64

typedef struct
{
	event_t events[MAXEVENTS];
	int eventhead;
	int eventtail;

} d_event_t;

//
// d_items
//
typedef struct
{
	weaponinfo_t	weaponinfo[NUMWEAPONS];
} d_items_t;

//
// d_main
//
typedef struct
{
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

} d_main_t;

//
// d_loop
//
typedef struct
{
	ticcmd_t cmds[NET_MAXPLAYERS];
	boolean ingame[NET_MAXPLAYERS];
} ticcmd_set_t;

// Callback function invoked while waiting for the netgame to start.
// The callback is invoked when new players are ready. The callback
// should return true, or return false to abort startup.

typedef boolean (*netgame_startup_callback_t)(int ready_players,
											  int num_players);

typedef struct
{
	// Read events from the event queue, and process them.

	void (*ProcessEvents)(void* data);

	// Given the current input state, fill in the fields of the specified
	// ticcmd_t structure with data for a new tic.

	void (*BuildTiccmd)(void* data, ticcmd_t *cmd, int maketic);

	// Advance the game forward one tic, using the specified player input.

	void (*RunTic)(void* data, ticcmd_t *cmds, boolean *ingame);

	// Run the menu (runs independently of the game).

	void (*RunMenu)(void* data);
} loop_interface_t;

typedef struct
{
	//
	// gametic is the tic about to (or currently being) run
	// maketic is the tic that hasn't had control made for it yet
	// recvtic is the latest tic received from the server.
	//
	// a gametic cannot be run until ticcmds are received for it
	// from all players.
	//

	ticcmd_set_t ticdata[BACKUPTICS];

	// The index of the next tic to be made (with a call to BuildTiccmd).

	int maketic;

	// The number of complete tics received from the server so far.

	int recvtic;

	// The number of tics that have been run (using RunTic) so far.

	int gametic;

	// When set to true, a single tic is run each time TryRunTics() is called.
	// This is used for -timedemo mode.

	boolean singletics;

	// Index of the local player.

	int localplayer;

	// Used for original sync code.

	int      skiptics;

	// Reduce the bandwidth needed by sampling game input less and transmitting
	// less.  If ticdup is 2, sample half normal, 3 = one third normal, etc.

	int		ticdup;

	// Amount to offset the timer for game sync.

	fixed_t         offsetms;

	// Use new client syncronisation code

	boolean  new_sync;

	// Callback functions for loop code.

	loop_interface_t *loop_interface;

	// Current players in the multiplayer game.
	// This is distinct from playeringame[] used by the game code, which may
	// modify playeringame[] when playing back multiplayer demos.

	boolean local_playeringame[NET_MAXPLAYERS];

	// Requested player class "sent" to the server on connect.
	// If we are only doing a single player game then this needs to be remembered
	// and saved in the game settings.

	int player_class;

} d_loop_t;

//
// f_finale
//
typedef struct
{
	char*	finaletext;
	char*	finaleflat;

} f_finale_t;

//
// f_wipe
//
typedef struct
{
	// when zero, stop the wipe
	boolean	go;

	byte*	wipe_scr_start;
	byte*	wipe_scr_end;
	byte*	wipe_scr;

} f_wipe_t;

//
// misc
//

typedef struct
{
	void*		user_data;

	uint32_t* 	DG_ScreenBuffer;

	// m_argv.c
	int				myargc;
	char**			myargv;

	// d_main.c
	d_main_t		d_main;
	am_map_t		am_map;
	d_event_t		d_event;
	d_items_t		d_items;
	d_loop_t		d_loop;
	ticcmd_t*		netcmds;
	f_finale_t		f_finale;
	f_wipe_t		f_wipe;

} data_t;

// Called by IO functions when input is detected.
void D_PostEvent (data_t* data, event_t *ev);

// Read an event from the event queue
event_t *D_PopEvent(data_t* data);

void D_Main_Init (data_t* data);
void AM_Map_Init (data_t* data);
void D_Items_Init (data_t* data);
void D_Loop_init (data_t* data);

#endif //DOOM_GENERIC
