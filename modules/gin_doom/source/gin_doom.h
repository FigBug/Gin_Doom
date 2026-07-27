#pragma once

class DoomComponent;

// Game setup for a CouchDoom multiplayer session, chosen on the title screen
// and applied by D_DoomMain. All instances in one session must pass identical
// values (the simulation is lockstep-deterministic).
struct DoomSetup
{
    int  deathmatch = 1;    // 0 = co-op, 1 = deathmatch, 2 = altdeath
    int  skill      = 2;    // skill_t: 0 = ITYTD .. 4 = Nightmare
    int  episode    = 1;    // shareware DOOM1.WAD => 1
    int  map        = 1;    // 1..9
    bool monsters   = true;
    int  fragLimit  = 0;    // 0 = unlimited
};

class Doom : private juce::Thread
{
public:
    Doom();
    ~Doom() override;

	void registerComponent (DoomComponent*);

    // Start the game. For CouchDoom, playerIndex/numPlayers configure this
    // instance as one player of an N-way local session (numPlayers > 1); the
    // default (numPlayers = 1) is a normal single-player game. playMusic false
    // starts with -nomusic (so only player 0 renders music). setup selects the
    // mode/map/skill/etc (used only when numPlayers > 1).
    // isBot makes this instance's player AI-controlled (a couch AI slot).
    void startGame (juce::File wadFile, int playerIndex = 0, int numPlayers = 1,
                    bool playMusic = true, DoomSetup setup = {}, bool isBot = false);
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
    bool            isBot = false;
    DoomSetup       setup;

    DoomAudioEngine audio;

	std::vector<std::pair<int, bool>> keyEvents;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Doom)
};
