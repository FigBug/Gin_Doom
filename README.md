# Gin_Doom

Embed **Doom** in a JUCE application as a `gin` module. Gin_Doom wraps a
[doomgeneric](https://github.com/ozkl/doomgeneric)-based Doom (Chocolate Doom
lineage) so it renders into a `juce::Image`, plays through JUCE audio, and takes
JUCE key events — drop it into a component and it runs.

Its distinguishing feature: the engine has been **de-globalized into per-instance
state** (`data_t`), so you can run **many independent Doom instances in a single
process** — each its own game on its own thread, with no shared globals. That's
what powers [CouchDoom](https://github.com/FigBug/CouchDoom)'s four-player local
split-screen deathmatch.

## Features

- Full Doom rendered to a 640×400 `juce::Image` you can draw anywhere.
- Pull-rendered audio — SFX plus OPL3 FM-synth music (Nuked OPL3).
- Keyboard input via JUCE key events.
- **Multiple concurrent instances** in one process (per-instance `data_t`).
- Optional lockstep "fake network" for local multiplayer deathmatch / co-op.

## Requirements

- JUCE, plus the `gin` and `gin_dsp` modules (see the `dependencies` line in
  `modules/gin_doom/gin_doom.h`).
- A C++17 compiler.
- A Doom IWAD to load at runtime (e.g. the shareware `DOOM1.WAD`). Gin_Doom ships
  no game data — you supply the WAD.

Add `gin_doom` alongside your other JUCE/gin modules (Projucer, or CMake
`juce_add_module`). Everything lives in the `gin` namespace.

## Quick start — drop-in component

Give a `gin::DoomComponent` a `gin::Doom`, make it visible (it wants 640×400),
start a game with a WAD, and pump its audio. The component registers itself and
drives its own repaints; it also handles keyboard input.

```cpp
// owned somewhere long-lived (e.g. your processor)
gin::Doom doom;

// in your editor / view
gin::DoomComponent doomComponent { doom };
addAndMakeVisible (doomComponent);
doomComponent.setBounds (0, 0, 640, 400);

// Doom runs on its own thread; this returns immediately
doom.startGame (wadFile);

// in your audio callback
doom.getAudioEngine().processBlock (buffer, (int) sampleRate);
```

## Programmatic use

For full control — custom rendering, several instances, feeding input yourself —
drive `gin::Doom` directly:

```cpp
gin::Doom doom;
doom.startGame (wadFile);

juce::Image frame = doom.getScreen();             // latest 640x400 framebuffer
doom.addEvent (juce::KeyPress::spaceKey, true);   // key down
doom.addEvent (juce::KeyPress::spaceKey, false);  // key up
```

| Method | Purpose |
|---|---|
| `startGame(wad, playerIndex, numPlayers, playMusic, setup, isBot)` | launch a game (all args after `wad` default to single-player) |
| `getScreen()` | latest framebuffer as a `juce::Image` (640×400) |
| `addEvent(key, isDown)` | feed a key event |
| `getAudioEngine()` | the instance's `DoomAudioEngine` |
| `registerComponent(c)` | attach a `DoomComponent` (the component's constructor does this for you) |

## Audio

Each instance owns a `DoomAudioEngine` that renders on demand. Call one of:

- `processBlock(buffer, sampleRate)` — SFX **and** music together (single instance).
- `processSfx(...)` / `processMusic(...)` — rendered separately, so a host can
  spatialise a player's SFX while keeping music centred (CouchDoom uses this for
  per-player panning).

Music is synthesised with Nuked OPL3, matching the classic AdLib/OPL soundtrack.

## Multiple instances

Because all game state lives in a per-instance `data_t`, you can construct several
`gin::Doom` objects and run them concurrently — no shared globals. Pass
`numPlayers > 1` to `startGame` to configure an instance as one player of a local
session; a built-in lockstep arbiter exchanges input each tic so the instances
stay in perfect sync — Doom's own peer-to-peer deathmatch model, with the network
replaced by a function call. Bots (`isBot`) can fill empty slots, and a
`gin::DoomSetup` selects mode / skill / map / monsters / frag limit.

All of this multiplayer machinery is **optional and gated**: a single instance
(`numPlayers == 1`) never touches the arbiter, bots, or deathmatch config, so the
drop-in component path above is completely unaffected.

## Credits & license

Built on [doomgeneric](https://github.com/ozkl/doomgeneric) and the Chocolate
Doom / id Software Doom source. Licensed under the **GPL v2** (see `LICENSE`); the
original id Software release notes are preserved in `README.TXT`.
