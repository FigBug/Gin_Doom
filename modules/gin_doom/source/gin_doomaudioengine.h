#pragma once

class DoomAudioEngine
{
public:
    DoomAudioEngine();
    ~DoomAudioEngine();

    void processBlock (juce::AudioBuffer<float>& buffer, int sampleRate);

    // SFX and music separately, so the CouchDoom mixer can spatialise this
    // player's SFX by screen position while keeping music centred. processBlock
    // renders both together (for standalone single-instance use).
    void processSfx   (juce::AudioBuffer<float>& buffer, int sampleRate);
    void processMusic (juce::AudioBuffer<float>& buffer, int sampleRate);

    void precacheSounds (void* sounds, int num_sounds);
    int getSfxLumpNum (void* sfx);
    void updateSoundParams (int handle, int vol, int sep);
    int startSound (void* sfxinfo, int channel, int vol, int sep);
    void stopSound (int handle);
    bool soundIsPlaying (int handle);
    void updateSound (void);
    void shutdownSound (void);
    bool initSound (bool _use_sfx_prefix);

    // Called by Doom once its per-instance data_t exists, so the audio
    // thread can locate this instance's OPL music state (data->opl_music).
    void attach (void* doomData);

private:
    juce::CriticalSection lock;

    // The owning instance's data_t (opaque here; cast in the .cpp). Used to
    // reach the per-instance music state on the audio thread.
    void* doomData = nullptr;

    // Scratch buffer for OPL output (stereo interleaved 16-bit).
    juce::HeapBlock<int16_t> musicScratch;
    int musicScratchSamples = 0;

    struct Channel
    {
        bool playing = false;
        int pos = 0;
        int samplerate = 0;
        float gainL = 1.0f;
        float gainR = 1.0f;

        juce::AudioSampleBuffer buffer {1, 1};
        gin::ResamplingFifo fifo { 16 };

        void processBlock (juce::AudioBuffer<float>& buffer, int sampleRate);
    };

    bool useSFXprefix = false;
    Channel channels[16];
};
