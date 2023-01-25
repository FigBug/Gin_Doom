
extern "C"
{
#include "../../../doomgeneric/m_argv.h"
#include "../../../doomgeneric/doomkeys.h"

void D_DoomMain (void);
void M_FindResponseFile(void);
void dg_Create();

extern uint32_t* DG_ScreenBuffer;
}

static DoomComponent* dc = nullptr;

int keyCodes[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'm', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    juce::KeyPress::spaceKey,
    juce::KeyPress::escapeKey,
    juce::KeyPress::returnKey,
    juce::KeyPress::tabKey,
    juce::KeyPress::deleteKey,
    juce::KeyPress::backspaceKey,
    juce::KeyPress::insertKey,
    juce::KeyPress::upKey,
    juce::KeyPress::downKey,
    juce::KeyPress::leftKey,
    juce::KeyPress::rightKey,
    juce::KeyPress::pageUpKey,
    juce::KeyPress::pageDownKey,
    juce::KeyPress::homeKey,
    juce::KeyPress::endKey,
    juce::KeyPress::F1Key,
    juce::KeyPress::F2Key,
    juce::KeyPress::F3Key,
    juce::KeyPress::F4Key,
    juce::KeyPress::F5Key,
    juce::KeyPress::F6Key,
    juce::KeyPress::F7Key,
    juce::KeyPress::F8Key,
    juce::KeyPress::F9Key,
    juce::KeyPress::F10Key,
    juce::KeyPress::F11Key,
    juce::KeyPress::F12Key,
    juce::KeyPress::F13Key,
    juce::KeyPress::F14Key,
    juce::KeyPress::F15Key,
    juce::KeyPress::F16Key,
    juce::KeyPress::F17Key,
    juce::KeyPress::F18Key,
    juce::KeyPress::F19Key,
    juce::KeyPress::F20Key,
    juce::KeyPress::F21Key,
    juce::KeyPress::F22Key,
    juce::KeyPress::F23Key,
    juce::KeyPress::F24Key,
    juce::KeyPress::F25Key,
    juce::KeyPress::F26Key,
    juce::KeyPress::F27Key,
    juce::KeyPress::F28Key,
    juce::KeyPress::F29Key,
    juce::KeyPress::F30Key,
    juce::KeyPress::F31Key,
    juce::KeyPress::F32Key,
    juce::KeyPress::F33Key,
    juce::KeyPress::F34Key,
    juce::KeyPress::F35Key,
    juce::KeyPress::numberPad0,
    juce::KeyPress::numberPad1,
    juce::KeyPress::numberPad2,
    juce::KeyPress::numberPad3,
    juce::KeyPress::numberPad4,
    juce::KeyPress::numberPad5,
    juce::KeyPress::numberPad6,
    juce::KeyPress::numberPad7,
    juce::KeyPress::numberPad8,
    juce::KeyPress::numberPad9,
    juce::KeyPress::numberPadAdd,
    juce::KeyPress::numberPadSubtract,
    juce::KeyPress::numberPadMultiply,
    juce::KeyPress::numberPadDivide,
    juce::KeyPress::numberPadSeparator,
    juce::KeyPress::numberPadDecimalPoint,
    juce::KeyPress::numberPadEquals,
    juce::KeyPress::numberPadDelete,
    juce::KeyPress::playKey,
    juce::KeyPress::stopKey,
    juce::KeyPress::fastForwardKey,
    juce::KeyPress::rewindKey
};

constexpr auto shift = 0x100001;
constexpr auto alt   = 0x100002;
constexpr auto ctrl  = 0x100003;

void updateFrame (juce::Image img)
{
    juce::ScopedLock sl (dc->lock);
    dc->screen = img;
    juce::MessageManager::callAsync ([]
    {
        if (dc)
            dc->repaint();
    });
}

std::optional<std::pair<int, bool>> getKeyEvent()
{
    juce::ScopedLock sl (dc->lock);
    if (dc->keyEvents.size() == 0)
        return {};

    auto evt = dc->keyEvents[0];
    dc->keyEvents.erase (dc->keyEvents.begin());

    return evt;
}

extern "C" void DG_Init()
{
}

extern "C" void DG_DrawFrame()
{
    if (dc == nullptr)
        return;

    juce::Image img (juce::Image::ARGB, 640, 400, true);

    juce::Image::BitmapData data (img, juce::Image::BitmapData::readOnly);

    for (int y = 0; y < 400; y++)
    {
        uint8_t* p = data.getLinePointer (y);

        for (int x = 0; x < 640; x++)
        {
            uint32_t px = DG_ScreenBuffer[y * 640 + x];

            uint8_t b = (px & 0xff000000) >> 24;
            uint8_t r = (px & 0x00ff0000) >> 16;
            uint8_t g = (px & 0x0000ff00) >> 8;
            uint8_t a = (px & 0x000000ff) >> 0;

            juce::PixelARGB* s = (juce::PixelARGB*)p;
            s->setARGB (a, r, g, b);

            p += data.pixelStride;
        }
    }

    updateFrame (img);
}

extern "C" void DG_SleepMs (uint32_t ms)
{
    juce::Thread::sleep (int (ms));
}

extern "C" uint32_t DG_GetTicksMs()
{
    static uint32_t startup = 0;
    if (startup == 0)
        startup = juce::Time::getMillisecondCounter();

    return juce::Time::getMillisecondCounter() - startup;
}

extern "C" int DG_GetKey (int* pressed, unsigned char* key)
{
    auto event = getKeyEvent();
    if (! event.has_value())
        return 0;

    *pressed = event->second ? 1 : 0;
    *key = (unsigned char)event->first;

    return 1;
}

extern "C" void DG_SetWindowTitle (const char* title)
{
    juce::ignoreUnused (title);
}

DoomComponent::DoomComponent()
    : juce::Thread ("Doom")
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    startTimerHz (60);
}

void DoomComponent::startGame (juce::File wadFile_)
{
    wadFile = wadFile_;
    startThread();
}

void DoomComponent::run()
{
    dc = this;

    auto path = wadFile.getFullPathName();

    const char* params[3];
    params[0] = "doom";
    params[1] = "-iwad";
    params[2] = path.toRawUTF8();

    myargc = 3;
    myargv = (char**)params;

    M_FindResponseFile();

    // start doom
    printf("Starting D_DoomMain\r\n");

    dg_Create();

    D_DoomMain ();

    dc = nullptr;
}

void DoomComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    juce::ScopedLock sl (lock);
    if (screen.isValid())
        g.drawImage (screen, getLocalBounds().toFloat());
}

void DoomComponent::timerCallback()
{
    keyStateChanged (false);
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
            addEvent (mapKey (k), false);

    //
    // Find pressed keys
    //
    for (auto k : currentKeys)
        if (! pressedKeys.contains (k))
            addEvent (mapKey (k), true);

    pressedKeys = currentKeys;

    return true;
}

void DoomComponent::addEvent (int key, bool press)
{
    juce::ScopedLock sl (lock);
    keyEvents.push_back({key, press});
}

int DoomComponent::mapKey (int key)
{
    if (key == juce::KeyPress::rightKey) return KEY_RIGHTARROW;
    if (key == juce::KeyPress::leftKey) return KEY_LEFTARROW;
    if (key == juce::KeyPress::upKey) return KEY_UPARROW;
    if (key == juce::KeyPress::downKey) return KEY_DOWNARROW;
    //if (key == juce::KeyPress::) return KEY_STRAFE_L;
    //if (key == juce::KeyPress::) return KEY_STRAFE_R;
    if (key == juce::KeyPress::spaceKey) return KEY_USE;
    if (key == ctrl) return KEY_FIRE;
    if (key == juce::KeyPress::escapeKey) return KEY_ESCAPE;
    if (key == juce::KeyPress::returnKey) return KEY_ENTER;
    if (key == juce::KeyPress::tabKey) return KEY_TAB;
    if (key == juce::KeyPress::F1Key) return KEY_F1;
    if (key == juce::KeyPress::F2Key) return KEY_F2;
    if (key == juce::KeyPress::F3Key) return KEY_F3;
    if (key == juce::KeyPress::F4Key) return KEY_F4;
    if (key == juce::KeyPress::F5Key) return KEY_F5;
    if (key == juce::KeyPress::F6Key) return KEY_F6;
    if (key == juce::KeyPress::F7Key) return KEY_F7;
    if (key == juce::KeyPress::F8Key) return KEY_F8;
    if (key == juce::KeyPress::F9Key) return KEY_F9;
    if (key == juce::KeyPress::F10Key) return KEY_F10;
    if (key == juce::KeyPress::F11Key) return KEY_F11;
    if (key == juce::KeyPress::F12Key) return KEY_F12;
    if (key == juce::KeyPress::backspaceKey) return KEY_BACKSPACE;
    //if (key == juce::KeyPress::) return KEY_PAUSE;
    if (key == '=') return KEY_EQUALS;
    if (key == '-') return KEY_MINUS;
    if (key == shift) return KEY_RSHIFT;
    if (key == ctrl) return KEY_RCTRL;
    if (key == alt) return KEY_RALT;
    if (key == alt) return KEY_LALT;
    //if (key == juce::KeyPress::) return KEY_CAPSLOCK;
    //if (key == juce::KeyPress::) return KEY_NUMLOCK;
    //if (key == juce::KeyPress::) return KEY_SCRLCK;
    //if (key == juce::KeyPress::) return KEY_PRTSCR;
    if (key == juce::KeyPress::homeKey) return KEY_HOME;
    if (key == juce::KeyPress::endKey) return KEY_END;
    if (key == juce::KeyPress::pageUpKey) return KEY_PGUP;
    if (key == juce::KeyPress::pageDownKey) return KEY_PGDN;
    if (key == juce::KeyPress::insertKey) return KEY_INS;
    if (key == juce::KeyPress::deleteKey) return KEY_DEL;
    if (key == juce::KeyPress::numberPad0) return KEYP_0;
    if (key == juce::KeyPress::numberPad1) return KEYP_1;
    if (key == juce::KeyPress::numberPad2) return KEYP_2;
    if (key == juce::KeyPress::numberPad3) return KEYP_3;
    if (key == juce::KeyPress::numberPad4) return KEYP_4;
    if (key == juce::KeyPress::numberPad5) return KEYP_5;
    if (key == juce::KeyPress::numberPad6) return KEYP_6;
    if (key == juce::KeyPress::numberPad7) return KEYP_7;
    if (key == juce::KeyPress::numberPad8) return KEYP_8;
    if (key == juce::KeyPress::numberPad9) return KEYP_9;
    if (key == juce::KeyPress::numberPadDivide) return KEYP_DIVIDE;
    if (key == juce::KeyPress::numberPadAdd) return KEYP_PLUS;
    if (key == juce::KeyPress::numberPadSubtract) return KEYP_MINUS;
    if (key == juce::KeyPress::numberPadMultiply) return KEYP_MULTIPLY;
    if (key == juce::KeyPress::numberPadDecimalPoint) return KEYP_PERIOD;
    if (key == juce::KeyPress::numberPadEquals) return KEYP_EQUALS;
    //if (key == juce::KeyPress::) return KEYP_ENTER;

    if (isalpha (key))
        return toupper (key);

    return key;
}
