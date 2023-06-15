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

	am_map_t		am_map;
	d_event_t		d_event;

} data_t;

// Called by IO functions when input is detected.
void D_PostEvent (data_t* data, event_t *ev);

// Read an event from the event queue
event_t *D_PopEvent(data_t* data);

#endif //DOOM_GENERIC
