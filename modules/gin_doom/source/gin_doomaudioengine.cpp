
DoomAudioEngine* e = nullptr;

void DoomAudioEngine::Channel::processBlock (juce::AudioBuffer<float>& bufferOut, int sampleRateOut)
{
    fifo.setResamplingRatio (samplerate, sampleRateOut);

    while (fifo.samplesReady() < buffer.getNumSamples())
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

    fifo.popAudioBufferAdding (bufferOut);
}

DoomAudioEngine::DoomAudioEngine()
{
    e = this;
}

DoomAudioEngine::~DoomAudioEngine()
{
    e = nullptr;
}

void DoomAudioEngine::processBlock (juce::AudioBuffer<float>& buffer, int sampleRate)
{
    juce::ScopedLock sl (lock);

    for (auto& ch : channels)
        if (ch.playing)
            ch.processBlock (buffer, sampleRate);
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

    return W_GetNumForName (buf);
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

    sfxinfo_t* sfxinfo = (sfxinfo_t*)sfxinfo_;

    auto data = (uint8_t*)W_CacheLumpNum (sfxinfo->lumpnum, PU_STATIC);
    auto lumplen = W_LumpLength ((unsigned int)sfxinfo->lumpnum);

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

    W_ReleaseLumpNum (sfxinfo->lumpnum);

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

static void I_JUCE_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    e->precacheSounds (sounds, num_sounds);
}

static int I_JUCE_GetSfxLumpNum(sfxinfo_t *sfx)
{
    return e->getSfxLumpNum (sfx);
}

static void I_JUCE_UpdateSoundParams(int handle, int vol, int sep)
{
    e->updateSoundParams(handle, vol, sep);
}

static int I_JUCE_StartSound(sfxinfo_t* sfxinfo, int channel, int vol, int sep)
{
    return e->startSound (sfxinfo, channel, vol, sep);
}

static void I_JUCE_StopSound(int handle)
{
    e->stopSound (handle);
}

static boolean I_JUCE_SoundIsPlaying(int handle)
{
    return e->soundIsPlaying (handle);
}

//
// Periodically called to update the sound system
//

static void I_JUCE_UpdateSound(void)
{
    return e->updateSound();
}

static void I_JUCE_ShutdownSound(void)
{
    return e->shutdownSound();
}

static boolean I_JUCE_InitSound(boolean _use_sfx_prefix)
{
    return e->initSound (_use_sfx_prefix);
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
