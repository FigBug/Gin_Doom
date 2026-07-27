#pragma once

class DoomComponent;

class Doom : private juce::Thread             
{
public:
    Doom();
    ~Doom() override;

	void registerComponent (DoomComponent*);

    // Start the game. For CouchDoom, playerIndex/numPlayers configure this
    // instance as one player of an N-way local deathmatch (numPlayers > 1);
    // the default (numPlayers = 1) is a normal single-player game. playMusic
    // false starts with -nomusic (so only player 0 renders music).
    void startGame (juce::File wadFile, int playerIndex = 0, int numPlayers = 1,
                    bool playMusic = true);
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
    bool            playMusic = true;
    int             couchIndex = 0;
    int             couchPlayers = 1;

    DoomAudioEngine audio;

	std::vector<std::pair<int, bool>> keyEvents;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Doom)
};
