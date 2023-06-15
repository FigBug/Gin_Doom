#pragma once

class DoomComponent;

class Doom : private juce::Thread             
{
public:
    Doom();
    ~Doom() override;

	void registerComponent (DoomComponent*);

    void startGame (juce::File wadFile);
	void addEvent (int key, bool press);
	juce::Image getScreen();
	int mapKey (int key);

    DoomAudioEngine& getAudioEngine()   { return audio; }

private:
    friend void updateFrame (Doom*, juce::Image img);
    friend std::optional<std::pair<int, bool>> getKeyEvent (Doom*);

    void run() override;

	juce::CriticalSection lock;

	void*			user_data;

	juce::Image 	screen;
	DoomComponent*	component = nullptr;
    juce::File 		wadFile;

    DoomAudioEngine audio;

	std::vector<std::pair<int, bool>> keyEvents;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Doom)
};
