#ifndef DATA
#define DATA

typedef struct data_s data_t;
struct channel_s;
struct musicinfo_s;

#include "d_mode.h"
#include "d_event.h"
#include "doomdef.h"
#include "d_player.h"
#include "r_defs.h"

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

	boolean         playeringame[MAXPLAYERS];
	player_t        players[MAXPLAYERS];

	gameaction_t    gameaction;
	boolean         sendpause;
	boolean         sendsave;
	wbstartstruct_t wminfo;
	gamestate_t     wipegamestate;
	ticcmd_t*       netcmds;

	boolean         automapactive;
	boolean         menuactive;
	int             bodyqueslot;

	// s_sound.c
	int             sfxVolume;
	int             musicVolume;
	int             snd_SfxVolume;
	boolean         mus_paused;
	struct channel_s* channels;
	struct musicinfo_s* mus_playing;

	// g_game.c input/demo state
	int             next_weapon;
	boolean         gamekeydown[256];   // NUMKEYS
	int             turnheld;
	boolean         mousearray[9];      // MAX_MOUSE_BUTTONS+1
	boolean*        mousebuttons;       // &mousearray[1], set in DG_Alloc
	int             dclicktime;
	boolean         dclickstate;
	int             dclicks;
	int             dclicktime2;
	boolean         dclickstate2;
	int             dclicks2;
	int             joyxmove;
	int             joyymove;
	int             joystrafemove;
	boolean         joyarray[21];       // MAX_JOY_BUTTONS+1
	boolean*        joybuttons;         // &joyarray[1], set in DG_Alloc
	int             savegameslot;
	char            savedescription[32];
	boolean         timingdemo;
	int             starttime;
	boolean         turbodetected[MAXPLAYERS];
	boolean         longtics;
	boolean         lowres_turn;
	boolean         netdemo;
	byte            consistancy[MAXPLAYERS][BACKUPTICS];
	int             mousex;
	int             mousey;

	// r_draw.c render params
	byte*           dc_colormap;   // lighttable_t*
	int             dc_x;
	int             dc_yl;
	int             dc_yh;
	fixed_t         dc_iscale;
	fixed_t         dc_texturemid;
	byte*           dc_source;
	byte*           dc_translation;
	int             ds_y;
	int             ds_x1;
	int             ds_x2;
	byte*           ds_colormap;   // lighttable_t*
	fixed_t         ds_xfrac;
	fixed_t         ds_yfrac;
	fixed_t         ds_xstep;
	fixed_t         ds_ystep;
	byte*           ds_source;

	// v_video.c
	byte*           dest_screen;
	int             dirtybox[4];

	// i_video.c
	byte*           I_VideoBuffer;

	// r_main.c view state
	int             viewangleoffset;
	byte*           fixedcolormap;   // lighttable_t*
	int             centerx;
	int             centery;
	fixed_t         centerxfrac;
	fixed_t         centeryfrac;
	fixed_t         projection;
	fixed_t         viewx;
	fixed_t         viewy;
	fixed_t         viewz;
	angle_t         viewangle;
	fixed_t         viewcos;
	fixed_t         viewsin;
	player_t*       viewplayer;
	int             extralight;

	// r_main.c render scratch
	int             validcount;
	int             framecount;
	int             sscount;
	int             linecount;
	int             loopcount;
	int             detailshift;
	angle_t         clipangle;
	int             viewangletox[FINEANGLES/2];
	angle_t         xtoviewangle[SCREENWIDTH+1];
	byte*           scalelight[16][48];      // [LIGHTLEVELS][MAXLIGHTSCALE]
	byte*           scalelightfixed[48];     // [MAXLIGHTSCALE]
	byte*           zlight[16][128];         // [LIGHTLEVELS][MAXLIGHTZ]
	boolean         setsizeneeded;
	int             setblocks;
	int             setdetail;

	// r_bsp.c
	drawseg_t       drawsegs[MAXDRAWSEGS];
	drawseg_t*      ds_p;
	cliprange_t*    newend;
	cliprange_t     solidsegs[32];   // MAXSEGS
	// r_draw.c
	byte*           viewimage;
	int             viewwidth;
	int             scaledviewwidth;
	int             viewheight;
	int             viewwindowx;
	int             viewwindowy;
	byte*           ylookup[832];    // MAXHEIGHT
	int             columnofs[1120]; // MAXWIDTH
	byte            translations[3][256];
	byte*           background_buffer;
	int             dccount;
	int             fuzzpos;
	byte*           translationtables;
	int             dscount;

	// level geometry (p_setup.c)
	int             numvertexes;
	vertex_t*       vertexes;
	int             numsegs;
	seg_t*          segs;
	int             numsectors;
	sector_t*       sectors;
	int             numsubsectors;
	subsector_t*    subsectors;
	int             numnodes;
	node_t*         nodes;
	int             numlines;
	line_t*         lines;
	int             numsides;
	side_t*         sides;


};

#endif //DOOM_GENERIC
