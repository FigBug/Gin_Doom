#pragma once

class DoomComponent : public juce::Component,
                      private juce::Thread
{
public:
    DoomComponent();

    juce::Image screen;
    juce::CriticalSection lock;

private:
    void run() override;
    void paint (juce::Graphics& g) override;
};
