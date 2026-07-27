//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	The not so system specific sound interface.
//


#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomtype.h"


//
// SoundFX struct.
//
typedef struct sfxinfo_struct	sfxinfo_t;

struct sfxinfo_struct
{
    // tag name, used for hexen.
    char *tagname;
    
    // lump name.  If we are running with use_sfx_prefix=true, a
    // 'DS' (or 'DP' for PC speaker sounds) is prepended to this.

    char name[9];

    // Sfx priority
    int priority;

    // referenced sound if a link
    sfxinfo_t *link;

    // pitch if a link
    int pitch;

    // volume if a link
    int volume;

    // this is checked every second to see if sound
    // can be thrown out (if 0, then decrement, if -1,
    // then throw out, if > 0, then it is in use)
    int usefulness;

    // lump number of sfx
    int lumpnum;		

    // Maximum number of channels that the sound can be played on 
    // (Heretic)
    int numchannels;

    // data used by the low level code
    void *driver_data;
};

//
// MusicInfo struct.
//
typedef struct musicinfo_s
{
    // up to 6-character name
    char *name;

    // lump number of music
    int lumpnum;
    
    // music data
    void *data;

    // music handle once registered
    void *handle;
    
} musicinfo_t;

typedef enum 
{
    SNDDEVICE_NONE = 0,
    SNDDEVICE_PCSPEAKER = 1,
    SNDDEVICE_ADLIB = 2,
    SNDDEVICE_SB = 3,
    SNDDEVICE_PAS = 4,
    SNDDEVICE_GUS = 5,
    SNDDEVICE_WAVEBLASTER = 6,
    SNDDEVICE_SOUNDCANVAS = 7,
    SNDDEVICE_GENMIDI = 8,
    SNDDEVICE_AWE32 = 9,
    SNDDEVICE_CD = 10,
} snddevice_t;

// Interface for sound modules

typedef struct
{
    // List of sound devices that this sound module is used for.

    snddevice_t *sound_devices;
    int num_sound_devices;

    // Initialise sound module
    // Returns true if successfully initialised

    boolean (*Init)(data_t* data, boolean use_sfx_prefix);

    // Shutdown sound module

    void (*Shutdown)(data_t* data);

    // Returns the lump index of the given sound.

    int (*GetSfxLumpNum)(data_t* data, sfxinfo_t *sfxinfo);

    // Called periodically to update the subsystem.

    void (*Update)(data_t* data);

    // Update the sound settings on the given channel.

    void (*UpdateSoundParams)(data_t* data, int channel, int vol, int sep);

    // Start a sound on a given channel.  Returns the channel id
    // or -1 on failure.

    int (*StartSound)(data_t* data, sfxinfo_t *sfxinfo, int channel, int vol, int sep);

    // Stop the sound playing on the given channel.

    void (*StopSound)(data_t* data, int channel);

    // Query if a sound is playing on the given channel

    boolean (*SoundIsPlaying)(data_t* data, int channel);

    // Called on startup to precache sound effects (if necessary)

    void (*CacheSounds)(data_t* data, sfxinfo_t *sounds, int num_sounds);

} sound_module_t;

void I_InitSound(data_t* data, boolean use_sfx_prefix);
void I_ShutdownSound(data_t* data);
int I_GetSfxLumpNum(data_t* data, sfxinfo_t *sfxinfo);
void I_UpdateSound(data_t* data);
void I_UpdateSoundParams(data_t* data, int channel, int vol, int sep);
int I_StartSound(data_t* data, sfxinfo_t *sfxinfo, int channel, int vol, int sep);
void I_StopSound(data_t* data, int channel);
boolean I_SoundIsPlaying(data_t* data, int channel);
void I_PrecacheSounds(data_t* data, sfxinfo_t *sounds, int num_sounds);

// Interface for music modules

typedef struct
{
    // List of sound devices that this music module is used for.

    snddevice_t *sound_devices;
    int num_sound_devices;

    // Initialise the music subsystem

    boolean (*Init)(data_t* data);

    // Shutdown the music subsystem

    void (*Shutdown)(data_t* data);

    // Set music volume - range 0-127

    void (*SetMusicVolume)(data_t* data, int volume);

    // Pause music

    void (*PauseMusic)(data_t* data);

    // Un-pause music

    void (*ResumeMusic)(data_t* data);

    // Register a song handle from song data
    // Returns a handle that can be used to play the song

    void *(*RegisterSong)(data_t* data, void *songdata, int len);

    // Un-register (free) song data

    void (*UnRegisterSong)(data_t* data, void *handle);

    // Play the song

    void (*PlaySong)(data_t* data, void *handle, boolean looping);

    // Stop playing the current song.

    void (*StopSong)(data_t* data);

    // Query if music is playing.

    boolean (*MusicIsPlaying)(data_t* data);

    // Invoked periodically to poll.

    void (*Poll)(data_t* data);
} music_module_t;

void I_InitMusic(data_t* data);
void I_ShutdownMusic(data_t* data);
void I_SetMusicVolume(data_t* data, int volume);
void I_PauseSong(data_t* data);
void I_ResumeSong(data_t* data);
void *I_RegisterSong(data_t* data, void *songdata, int len);
void I_UnRegisterSong(data_t* data, void *handle);
void I_PlaySong(data_t* data, void *handle, boolean looping);
void I_StopSong(data_t* data);
boolean I_MusicIsPlaying(data_t* data);

extern int snd_sfxdevice;
extern int snd_musicdevice;
extern int snd_samplerate;
extern int snd_cachesize;
extern int snd_maxslicetime_ms;
extern char *snd_musiccmd;

void I_BindSoundVariables(void);

#endif

