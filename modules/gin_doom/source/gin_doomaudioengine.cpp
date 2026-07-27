
//==============================================================================
// OPL music (i_oplmusic.c / opl.c).
//
// Each plugin instance owns its own OPL chip + sequencer, stored in its
// data_t (data->opl_music) and created on the game thread by I_OPL_InitMusic.
// The audio thread renders it here via DG_OPL_Render with an explicit handle;
// the OPL context has its own internal recursive lock, so no global state is
// involved.

extern "C" void DG_OPL_Render (void* music_handle, int16_t* buffer,
                               unsigned int nsamples, unsigned int rate);

// Relative level of the music mix. The Nuked OPL output is quiet - it peaks
// around ~9% of full-scale 16-bit for typical tracks - so it is amplified (not
// attenuated) to sit as an audible bed under the sound effects, with headroom
// left for louder passages and the master level.
static constexpr float kMusicGain = 3.0f;

void DoomAudioEngine::Channel::processBlock (juce::AudioBuffer<float>& bufferOut, int sampleRateOut)
{
    fifo.setResamplingRatio (samplerate, sampleRateOut);

    const int needed = bufferOut.getNumSamples();

    // Push input until the FIFO can supply this output block. Bound by the
    // output block size (not the whole sound) - otherwise the FIFO over-buffers,
    // truncates the tail, and can overflow on longer sounds, audible as
    // clicks/dropouts.
    while (fifo.samplesReady() < needed)
    {
        int todo = std::min (16, buffer.getNumSamples() - pos);
        if (todo == 0)
        {
            gin::ScratchBuffer silence (2, 128);
            silence.clear();
            fifo.pushAudioBuffer (silence);
            playing = false;
        }
        else
        {
            auto slice = gin::sliceBuffer (buffer, pos, todo);
            fifo.pushAudioBuffer (slice);
            pos += todo;
        }
    }

    // Pop the resampled block and mix it in with Doom's per-sound volume and
    // stereo separation (gainL/gainR). Applying them here honours distance/pan
    // instead of every sound playing at full scale (which summed across channels
    // and instances into constant clipping).
    gin::ScratchBuffer scratch (2, needed);
    fifo.popAudioBuffer (scratch);

    bufferOut.addFrom (0, 0, scratch, 0, 0, needed, gainL);
    if (bufferOut.getNumChannels() > 1)
        bufferOut.addFrom (1, 0, scratch, 1, 0, needed, gainR);
}

DoomAudioEngine::DoomAudioEngine()
{
}

DoomAudioEngine::~DoomAudioEngine()
{
}

void DoomAudioEngine::processBlock (juce::AudioBuffer<float>& buffer, int sampleRate)
{
    {
        juce::ScopedLock sl (lock);

        for (auto& ch : channels)
            if (ch.playing)
                ch.processBlock (buffer, sampleRate);
    }

    renderMusic (buffer, sampleRate);
}

void DoomAudioEngine::attach (void* data)
{
    juce::ScopedLock sl (lock);
    doomData = data;
}

void DoomAudioEngine::renderMusic (juce::AudioBuffer<float>& buffer, int sampleRate)
{
    if (buffer.getNumChannels() < 1)
        return;

    // Locate this instance's music state. The handle is published once on the
    // game thread (I_OPL_InitMusic) and cleared on shutdown.
    data_t* d;
    {
        juce::ScopedLock sl (lock);
        d = (data_t*) doomData;
    }

    if (d == nullptr)
        return;

    void* music = d->opl_music;
    if (music == nullptr)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    // Grow the scratch buffer if needed (rare; not the steady state).
    if (musicScratchSamples < n)
    {
        musicScratch.malloc (n * 2);
        musicScratchSamples = n;
    }

    // Renders this instance's OPL chip at the host rate (stereo interleaved).
    DG_OPL_Render (music, musicScratch.get(), (unsigned int) n,
                   (unsigned int) sampleRate);

    const float gain = kMusicGain / 32768.0f;

    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        l[i] += musicScratch[i * 2] * gain;

        if (r != nullptr)
            r[i] += musicScratch[i * 2 + 1] * gain;
    }
}

void DoomAudioEngine::precacheSounds (void* sounds, int num_sounds)
{
    juce::ignoreUnused (sounds, num_sounds);
}

int DoomAudioEngine::getSfxLumpNum (void* sfx_)
{
    sfxinfo_t* sfx = (sfxinfo_t*)sfx_;
    char buf[9];
    auto buf_len = sizeof(buf);

    if (sfx->link != nullptr)
        sfx = sfx->link;

    // Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
    // do this.
    if (useSFXprefix)
        M_snprintf (buf, buf_len, "ds%s", DEH_String (sfx->name));
    else
        M_StringCopy (buf, DEH_String (sfx->name), buf_len);

    return doomData != nullptr ? W_GetNumForName ((data_t*) doomData, buf) : 0;
}

void DoomAudioEngine::updateSoundParams (int handle, int vol, int sep)
{
    if (handle < 0 || handle >= int (std::size (channels)))
        return;

    int left = ((254 - sep) * vol) / 127;
    int right = ((sep) * vol) / 127;

    if (left < 0)
        left = 0;
    else if ( left > 255)
        left = 255;

    if (right < 0)
        right = 0;
    else if (right > 255)
        right = 255;

    channels[handle].gainL = left  / 255.0f;
    channels[handle].gainR = right / 255.0f;
}

int DoomAudioEngine::startSound (void* sfxinfo_, int channel, int vol, int sep)
{
    juce::ScopedLock sl (lock);

    if (channel < 0 || channel >= int (std::size (channels)))
        return 0;

    auto* dd = (data_t*) doomData;
    if (dd == nullptr)
        return 0;

    sfxinfo_t* sfxinfo = (sfxinfo_t*)sfxinfo_;

    auto data = (uint8_t*)W_CacheLumpNum (dd, sfxinfo->lumpnum, PU_STATIC);
    auto lumplen = W_LumpLength (dd, (unsigned int)sfxinfo->lumpnum);

    if (lumplen < 8 || data[0] != 0x03 || data[1] != 0x00)
        return -1;

    auto samplerate = (data[3] << 8) | data[2];
    auto length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

    if (length > lumplen - 8 || length <= 48)
        return -1;

    data += 16;
    length -= 32;

    auto& ch = channels[channel];
    ch.playing = true;
    ch.pos = 0;
    ch.samplerate = samplerate;
    ch.buffer.setSize (2, length, false, false, true);
    ch.fifo.reset();

    auto l = ch.buffer.getWritePointer (0);
    auto r = ch.buffer.getWritePointer (1);

    for (int i = 0; i < length; i++)
    {
        l[i] = (data[i] / 127.5f - 1.0f) * 0.65f;
        r[i] = (data[i] / 127.5f - 1.0f) * 0.65f;
    }

    W_ReleaseLumpNum (dd, sfxinfo->lumpnum);

    updateSoundParams (channel, vol, sep);

    return channel;
}

void DoomAudioEngine::stopSound (int handle)
{
    juce::ScopedLock sl (lock);

    if (handle < 0 || handle >= int (std::size (channels)))
        return;

    channels[handle].playing = false;
}

bool DoomAudioEngine::soundIsPlaying (int handle)
{
    juce::ScopedLock sl (lock);
    
    if (handle < 0 || handle >= int (std::size (channels)))
        return false;

    return channels[handle].playing;
}

void DoomAudioEngine::updateSound (void)
{
}

void DoomAudioEngine::shutdownSound (void)
{
}

bool DoomAudioEngine::initSound (bool _use_sfx_prefix)
{
    useSFXprefix = _use_sfx_prefix;
    return true;
}

// The SFX C callbacks receive the instance's data_t and resolve their engine
// from it (Doom::run stores &audio in data->audio_engine).

static DoomAudioEngine* engineFor (data_t* data)
{
    return data != nullptr ? (DoomAudioEngine*) data->audio_engine : nullptr;
}

static void I_JUCE_PrecacheSounds(data_t* data, sfxinfo_t *sounds, int num_sounds)
{
    if (auto* eng = engineFor (data))
        eng->precacheSounds (sounds, num_sounds);
}

static int I_JUCE_GetSfxLumpNum(data_t* data, sfxinfo_t *sfx)
{
    if (auto* eng = engineFor (data))
        return eng->getSfxLumpNum (sfx);
    return 0;
}

static void I_JUCE_UpdateSoundParams(data_t* data, int handle, int vol, int sep)
{
    if (auto* eng = engineFor (data))
        eng->updateSoundParams(handle, vol, sep);
}

static int I_JUCE_StartSound(data_t* data, sfxinfo_t* sfxinfo, int channel, int vol, int sep)
{
    if (auto* eng = engineFor (data))
        return eng->startSound (sfxinfo, channel, vol, sep);
    return -1;
}

static void I_JUCE_StopSound(data_t* data, int handle)
{
    if (auto* eng = engineFor (data))
        eng->stopSound (handle);
}

static boolean I_JUCE_SoundIsPlaying(data_t* data, int handle)
{
    if (auto* eng = engineFor (data))
        return eng->soundIsPlaying (handle);
    return false;
}

//
// Periodically called to update the sound system
//

static void I_JUCE_UpdateSound(data_t* data)
{
    if (auto* eng = engineFor (data))
        eng->updateSound();
}

static void I_JUCE_ShutdownSound(data_t* data)
{
    if (auto* eng = engineFor (data))
        eng->shutdownSound();
}

static boolean I_JUCE_InitSound(data_t* data, boolean _use_sfx_prefix)
{
    if (auto* eng = engineFor (data))
        return eng->initSound (_use_sfx_prefix);
    return false;
}

static snddevice_t sound_devices[] =
{
    SNDDEVICE_SB,
    SNDDEVICE_PAS,
    SNDDEVICE_GUS,
    SNDDEVICE_WAVEBLASTER,
    SNDDEVICE_SOUNDCANVAS,
    SNDDEVICE_AWE32,
};

extern "C" sound_module_t DG_sound_module =
{
    sound_devices,
    arrlen(sound_devices),
    I_JUCE_InitSound,
    I_JUCE_ShutdownSound,
    I_JUCE_GetSfxLumpNum,
    I_JUCE_UpdateSound,
    I_JUCE_UpdateSoundParams,
    I_JUCE_StartSound,
    I_JUCE_StopSound,
    I_JUCE_SoundIsPlaying,
    I_JUCE_PrecacheSounds,
};
