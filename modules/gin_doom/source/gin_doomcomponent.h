#pragma once

class DoomComponent : public juce::Component,
                      private juce::Thread,
                      private juce::Timer
{
public:
    DoomComponent();
    ~DoomComponent() override;

    void startGame (juce::File wadFile);

    DoomAudioEngine& getAudioEngine()   { return audio; }

private:
    friend void updateFrame (juce::Image img);
    friend std::optional<std::pair<int, bool>> getKeyEvent();

    void run() override;
    void paint (juce::Graphics& g) override;
    bool keyStateChanged (bool isKeyDown) override;
    void timerCallback() override;

    void addEvent (int key, bool press);
    int mapKey (int key);

    juce::Image screen;
    juce::CriticalSection lock;

    std::set<int> pressedKeys;
    std::vector<std::pair<int, bool>> keyEvents;

    juce::File wadFile;

    DoomAudioEngine audio;
};
