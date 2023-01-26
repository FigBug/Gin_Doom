#pragma once

class DoomAudioEngine
{
public:
    DoomAudioEngine();
    ~DoomAudioEngine();

    void precacheSounds (void* sounds, int num_sounds);
    int getSfxLumpNum (void* sfx);
    void updateSoundParams (int handle, int vol, int sep);
    int startSound (void* sfxinfo, int channel, int vol, int sep);
    void stopSound (int handle);
    bool soundIsPlaying (int handle);
    void updateSound (void);
    void shutdownSound (void);
    bool initSound (bool _use_sfx_prefix);

private:
    struct Channel
    {
        bool playing = false;
        int pos = 0;
        int samplerate = 0;
        float gainL = 1.0f;
        float gainR = 1.0f;

        juce::AudioSampleBuffer buffer {1, 1};
    };

    bool useSFXprefix = false;
    Channel channels[16];
};
