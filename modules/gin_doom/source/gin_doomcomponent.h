#pragma once

class DoomComponent : public juce::Component
					, private juce::Timer
{
public:
    DoomComponent (Doom&);
    ~DoomComponent() override;

	void timerCallback() override;

private:
    friend void updateFrame (juce::Image img);
    friend std::optional<std::pair<int, bool>> getKeyEvent();

    void paint (juce::Graphics& g) override;
    bool keyStateChanged (bool isKeyDown) override;

	Doom&			doom;
    std::set<int> 	pressedKeys;
};
