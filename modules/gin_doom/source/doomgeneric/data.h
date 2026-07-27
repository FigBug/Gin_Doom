#ifndef DATA
#define DATA

typedef struct data_s data_t;
struct channel_s;
struct texture_s;
struct menu_s;
typedef struct menu_s menu_t;
typedef struct texture_s texture_t;
struct musicinfo_s;

#include "d_mode.h"
#include "d_event.h"
#include "doomdef.h"
#include "d_player.h"
#include "r_defs.h"
#include "hu_lib.h"
#include "st_lib.h"
#include "p_spec.h"

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

	// r_plane.c
	visplane_t      visplanes[128];          // MAXVISPLANES
	visplane_t*     lastvisplane;
	visplane_t*     floorplane;
	visplane_t*     ceilingplane;
	short           openings[SCREENWIDTH*64];  // MAXOPENINGS
	short*          lastopening;
	short           floorclip[SCREENWIDTH];
	short           ceilingclip[SCREENWIDTH];
	int             spanstart[SCREENHEIGHT];
	int             spanstop[SCREENHEIGHT];
	byte**          planezlight;             // lighttable_t**
	fixed_t         planeheight;
	fixed_t         yslope[SCREENHEIGHT];
	fixed_t         distscale[SCREENWIDTH];
	fixed_t         basexscale;
	fixed_t         baseyscale;
	fixed_t         cachedheight[SCREENHEIGHT];
	fixed_t         cacheddistance[SCREENHEIGHT];
	fixed_t         cachedxstep[SCREENHEIGHT];
	fixed_t         cachedystep[SCREENHEIGHT];
	// r_things.c
	fixed_t         pspritescale;
	fixed_t         pspriteiscale;
	byte**          spritelights;            // lighttable_t**
	short           negonearray[SCREENWIDTH];
	short           screenheightarray[SCREENWIDTH];
	int             numsprites;
	int             maxframe;
	vissprite_t     vissprites[128];         // MAXVISSPRITES
	vissprite_t*    vissprite_p;
	int             newvissprite;
	vissprite_t     overflowsprite;
	short*          mfloorclip;
	short*          mceilingclip;
	fixed_t         spryscale;
	fixed_t         sprtopscreen;
	vissprite_t     vsprsortedhead;
	short           clipbot[SCREENWIDTH];
	short           cliptop[SCREENWIDTH];
	// r_segs.c
	boolean         segtextured;
	boolean         markfloor;
	boolean         markceiling;
	boolean         maskedtexture;
	int             toptexture;
	int             bottomtexture;
	int             midtexture;
	angle_t         rw_normalangle;
	int             rw_angle1;
	int             rw_x;
	int             rw_stopx;
	angle_t         rw_centerangle;
	fixed_t         rw_offset;
	fixed_t         rw_distance;
	fixed_t         rw_scale;
	fixed_t         rw_scalestep;
	fixed_t         rw_midtexturemid;
	fixed_t         rw_toptexturemid;
	fixed_t         rw_bottomtexturemid;
	int             worldtop;
	int             worldbottom;
	int             worldhigh;
	int             worldlow;
	fixed_t         pixhigh;
	fixed_t         pixlow;
	fixed_t         pixhighstep;
	fixed_t         pixlowstep;
	fixed_t         topfrac;
	fixed_t         topstep;
	fixed_t         bottomfrac;
	fixed_t         bottomstep;
	byte**          walllights;              // lighttable_t**
	short*          maskedtexturecol;

	// p_map.c
	fixed_t         tmbbox[4];
	mobj_t*         tmthing;
	int             tmflags;
	fixed_t         tmx;
	fixed_t         tmy;
	boolean         floatok;
	fixed_t         tmfloorz;
	fixed_t         tmceilingz;
	fixed_t         tmdropoffz;
	line_t*         ceilingline;
	line_t*         spechit[20];    // MAXSPECIALCROSS
	int             numspechit;
	fixed_t         bestslidefrac;
	fixed_t         secondslidefrac;
	line_t*         bestslideline;
	line_t*         secondslideline;
	mobj_t*         slidemo;
	fixed_t         tmxmove;
	fixed_t         tmymove;
	mobj_t*         shootthing;
	mobj_t*         linetarget;
	fixed_t         shootz;
	int             la_damage;
	fixed_t         attackrange;
	fixed_t         aimslope;
	mobj_t*         usething;
	mobj_t*         bombsource;
	mobj_t*         bombspot;
	int             bombdamage;
	boolean         crushchange;
	boolean         nofit;

	// p_maputl.c
	fixed_t         opentop;
	fixed_t         openbottom;
	fixed_t         openrange;
	fixed_t         lowfloor;
	intercept_t     intercepts[189];  // MAXINTERCEPTS
	intercept_t*    intercept_p;
	divline_t       trace;
	boolean         earlyout;
	int             ptflags;

	// p_tick.c
	thinker_t       thinkercap;
	// p_setup.c
	int             totallines;
	int             bmapwidth;
	int             bmapheight;
	short*          blockmap;
	short*          blockmaplump;
	fixed_t         bmaporgx;
	fixed_t         bmaporgy;
	mobj_t**        blocklinks;
	byte*           rejectmatrix;
	// r_sky.c
	int             skyflatnum;
	int             skytexture;
	int             skytexturemid;
	// p_user.c
	boolean         onground;
	// p_mobj.c
	int             itemrespawntime[128];  // ITEMQUESIZE
	int             iquehead;
	int             iquetail;
	// p_pspr.c
	fixed_t         swingx;
	fixed_t         swingy;
	fixed_t         bulletslope;

	// r_data.c
	int             firstflat;
	int             lastflat;
	int             numflats;
	int             firstpatch;
	int             lastpatch;
	int             numpatches;
	int             firstspritelump;
	int             lastspritelump;
	int             numspritelumps;
	int             numtextures;
	texture_t**     textures;
	texture_t**     textures_hashtable;
	int*            texturewidthmask;
	fixed_t*        textureheight;
	int*            texturecompositesize;
	short**         texturecolumnlump;
	byte**          texturecomposite;
	int*            flattranslation;
	int*            texturetranslation;
	fixed_t*        spritewidth;
	fixed_t*        spriteoffset;
	fixed_t*        spritetopoffset;
	byte*           colormaps;   // lighttable_t*
	int             flatmemory;
	int             texturememory;
	int             spritememory;

	// p_setup.c spawns + m_menu config
	mapthing_t      deathmatchstarts[10];  // MAX_DEATHMATCH_STARTS
	mapthing_t*     deathmatch_p;
	mapthing_t      playerstarts[MAXPLAYERS];
	int             mouseSensitivity;

	// game identity (doomstat.c)
	GameMode_t      gamemode;
	GameMission_t   gamemission;
	GameVersion_t   gameversion;
	char*           gamedescription;
	boolean         modifiedgame;

	boolean         testcontrols;
	int             testcontrols_mousespeed;
	int             snd_channels;

	// am_map.c
	int             am_cheating;
	int             am_grid;
	int             am_finit_width;
	int             am_finit_height;
	int             am_f_x;
	int             am_f_y;
	int             am_f_w;
	int             am_f_h;
	int             am_lightlev;
	byte*           am_fb;
	int             am_amclock;
	mpoint_t        am_m_paninc;
	fixed_t         am_mtof_zoommul;
	fixed_t         am_ftom_zoommul;
	fixed_t         am_m_x;
	fixed_t         am_m_y;
	fixed_t         am_m_x2;
	fixed_t         am_m_y2;
	fixed_t         am_m_w;
	fixed_t         am_m_h;
	fixed_t         am_min_x;
	fixed_t         am_min_y;
	fixed_t         am_max_x;
	fixed_t         am_max_y;
	fixed_t         am_max_w;
	fixed_t         am_max_h;
	fixed_t         am_min_w;
	fixed_t         am_min_h;
	fixed_t         am_min_scale_mtof;
	fixed_t         am_max_scale_mtof;
	fixed_t         am_old_m_w;
	fixed_t         am_old_m_h;
	fixed_t         am_old_m_x;
	fixed_t         am_old_m_y;
	mpoint_t        am_f_oldloc;
	fixed_t         am_scale_mtof;
	fixed_t         am_scale_ftom;
	patch_t*        am_marknums[10];
	mpoint_t        am_markpoints[10];
	int             am_markpointnum;
	int             am_followplayer;
	boolean         am_stopped;
	int             am_leveljuststarted;
	player_t*       am_plr;

	// wi_stuff.c
	int             wi_acceleratestage;
	int             wi_me;
	int             wi_state;
	wbstartstruct_t* wi_wbs;
	wbplayerstruct_t* wi_plrs;
	int             wi_cnt;
	int             wi_bcnt;
	int             wi_firstrefresh;
	int             wi_cnt_kills[MAXPLAYERS];
	int             wi_cnt_items[MAXPLAYERS];
	int             wi_cnt_secret[MAXPLAYERS];
	int             wi_cnt_time;
	int             wi_cnt_par;
	int             wi_cnt_pause;
	int             wi_dm_state;
	int             wi_dm_frags[MAXPLAYERS][MAXPLAYERS];
	int             wi_dm_totals[MAXPLAYERS];
	int             wi_cnt_frags[MAXPLAYERS];
	int             wi_dofrags;
	int             wi_ng_state;
	int             wi_sp_state;
	boolean         wi_snl_pointeron;

	// hu_stuff.c
	hu_textline_t   hu_w_title;
	hu_itext_t      hu_w_chat;
	boolean         hu_always_off;
	char            hu_chat_dest[MAXPLAYERS];
	hu_itext_t      hu_w_inputbuffer[MAXPLAYERS];
	boolean         hu_message_on;
	boolean         hu_message_nottobefuckedwith;
	hu_stext_t      hu_w_message;
	int             hu_message_counter;
	boolean         hu_headsupactive;
	player_t*       hu_plr;
	char            hu_chatchars[128];   // QUEUESIZE
	int             hu_head;
	int             hu_tail;

	// st_stuff.c
	player_t*       sb_plyr;
	boolean         sb_st_firsttime;
	int             sb_lu_palette;
	int             sb_st_msgcounter;
	int             sb_st_chatstate;
	int             sb_st_gamestate;
	boolean         sb_st_statusbaron;
	boolean         sb_st_chat;
	boolean         sb_st_oldchat;
	boolean         sb_st_cursoron;
	boolean         sb_st_notdeathmatch;
	boolean         sb_st_armson;
	boolean         sb_st_fragson;
	unsigned int    sb_st_clock;
	st_number_t     sb_w_ready;
	st_number_t     sb_w_frags;
	st_percent_t    sb_w_health;
	st_binicon_t    sb_w_armsbg;
	st_multicon_t   sb_w_arms[6];
	st_multicon_t   sb_w_faces;
	st_multicon_t   sb_w_keyboxes[3];
	st_percent_t    sb_w_armor;
	st_number_t     sb_w_ammo[4];
	st_number_t     sb_w_maxammo[4];
	int             sb_st_fragscount;
	int             sb_st_oldhealth;
	boolean         sb_oldweaponsowned[NUMWEAPONS];
	int             sb_st_facecount;
	int             sb_st_faceindex;
	int             sb_keyboxes[3];
	int             sb_st_randomnumber;
	int             sb_st_palette;
	boolean         sb_st_stopped;

	// m_menu.c
	menu_t*         mn_currentMenu;
	short           mn_itemOn;
	short           mn_skullAnimCounter;
	short           mn_whichSkull;
	int             mn_showMessages;
	int             mn_detailLevel;
	int             mn_screenblocks;
	int             mn_screenSize;
	int             mn_quickSaveSlot;
	int             mn_messageToPrint;
	char*           mn_messageString;
	int             mn_messx;
	int             mn_messy;
	int             mn_messageLastMenuActive;
	boolean         mn_messageNeedsInput;
	void            (*mn_messageRoutine)(data_t*, int);
	int             mn_saveStringEnter;
	int             mn_saveSlot;
	int             mn_saveCharIndex;
	char            mn_saveOldString[24];
	boolean         mn_inhelpscreens;
	char            mn_savegamestrings[10][24];
	int             mn_epi;

	// f_finale.c
	char*           finaletext;
	char*           finaleflat;
	int             finalestage;
	unsigned int    finalecount;
	int             castnum;
	int             casttics;
	boolean         castdeath;
	int             castframes;
	int             castonmelee;
	boolean         castattacking;
	// p_enemy.c
	mobj_t*         soundtarget;
	mobj_t*         corpsehit;
	mobj_t*         vileobj;
	fixed_t         viletryx;
	fixed_t         viletryy;
	mobj_t*         braintargets[32];
	int             numbraintargets;
	int             braintargeton;
	// p_sight.c
	fixed_t         sightzstart;
	fixed_t         si_topslope;
	fixed_t         si_bottomslope;
	int             sightcounts[2];
	fixed_t         t2x;
	fixed_t         t2y;

	// d_loop.c
	int             frameon;
	int             frameskip[4];
	int             lasttime;
	boolean         local_playeringame[NET_MAXPLAYERS];
	int             localplayer;
	int             maketic;
	boolean         new_sync;
	fixed_t         offsetms;
	int             oldnettics;
	int             player_class;
	int             recvtic;
	boolean         singletics;
	int             skiptics;
	int             ticdup;
	// g_game.c demo/game state
	char*           demoname;
	byte*           demobuffer;
	byte*           demo_p;
	byte*           demoend;
	boolean         secretexit;
	char            savename[256];
	int             d_episode;
	int             d_map;

	// p_spec/switch/plats/ceilng active lists
	short           numlinespecials;
	line_t*         linespeciallist[64];  // MAXLINEANIMS
	button_t        buttonlist[16];       // MAXBUTTONS
	ceiling_t*      activeceilings[30];   // MAXCEILINGS
	plat_t*         activeplats[30];      // MAXPLATS
	// i_video/i_input
	int             usegamma;
	boolean         screenvisible;
	int             shiftdown;

	// d_main title/demo sequence
	int             dm_demosequence;
	int             dm_pagetic;
	char*           dm_pagename;
	char            dm_title[128];
	// g_game
	mobj_t*         bodyque[32];   // BODYQUESIZE
	char*           defdemoname;
	// hu_stuff
	boolean         hu_chat_on;
	boolean         hu_message_dontfuckwithme;

	// d_main D_Display render-refresh state (was function-local statics)
	boolean         dd_viewactivestate;
	boolean         dd_menuactivestate;
	boolean         dd_inhelpscreensstate;
	boolean         dd_fullscreen;
	gamestate_t     dd_oldgamestate;
	int             dd_borderdrawcount;

	// function-local statics moved out
	char            hu_lastmessage[HU_MAXLINELENGTH+1];
	boolean         hu_altdown;
	int             hu_num_nobrainers;
	int             sb_pain_lastcalc;
	int             sb_pain_oldhealth;
	int             fin_laststage;
	int             pe_easy;
	int             dl_oldentertics;

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
