DoomComponent::DoomComponent (Doom& d)
	: doom (d)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

	startTimerHz (60);

	doom.registerComponent (this);
}

DoomComponent::~DoomComponent()
{
	doom.registerComponent (nullptr);
}

void DoomComponent::timerCallback()
{
	keyStateChanged (false);
}

void DoomComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

	auto screen = doom.getScreen();
    if (screen.isValid())
        g.drawImage (screen, getLocalBounds().toFloat());
}

bool DoomComponent::keyStateChanged (bool)
{
    std::set<int> currentKeys;

    for (auto k : keyCodes)
        if (juce::KeyPress::isKeyCurrentlyDown (k))
            currentKeys.insert (k);

    auto& mods = juce::ModifierKeys::currentModifiers;
    if (mods.isShiftDown())     currentKeys.insert (shift);
    if (mods.isCommandDown())   currentKeys.insert (ctrl);
    if (mods.isCtrlDown())      currentKeys.insert (ctrl);
    if (mods.isAltDown())       currentKeys.insert (alt);

    //
    // Find released keys
    //
    for (auto k : pressedKeys)
        if (! currentKeys.contains (k))
            doom.addEvent (doom.mapKey (k), false);

    //
    // Find pressed keys
    //
    for (auto k : currentKeys)
        if (! pressedKeys.contains (k))
            doom.addEvent (doom.mapKey (k), true);

    pressedKeys = currentKeys;

    return true;
}
