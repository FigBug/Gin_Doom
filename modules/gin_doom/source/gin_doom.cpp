
extern "C"
{
	data_t* DG_Alloc();
	void DG_Free(data_t* data);

	void D_DoomMain (data_t* data);
	void M_FindResponseFile (data_t* data);
	void dg_Create (data_t* data);

	extern uint32_t* DG_ScreenBuffer;
	extern uint32_t runloop;
}

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

void updateFrame (Doom* doom, juce::Image img)
{
    juce::ScopedLock sl (doom->lock);
	doom->screen = img;
    juce::MessageManager::callAsync ([doom, safe = juce::Component::SafePointer<juce::Component> (doom->component)]
    {
        if (safe && doom)
			safe->repaint();
    });
}

std::optional<std::pair<int, bool>> getKeyEvent (Doom* doom)
{
    juce::ScopedLock sl (doom->lock);
    if (doom->keyEvents.size() == 0)
        return {};

    auto evt = doom->keyEvents[0];
	doom->keyEvents.erase (doom->keyEvents.begin());

    return evt;
}

extern "C" void DG_Init (data_t* /*data*/)
{
}

extern "C" void DG_DrawFrame (data_t* data)
{
	auto doom = (Doom*)data->user_data;

    if (doom == nullptr)
        return;

    juce::Image img (juce::Image::ARGB, 640, 400, true);

    juce::Image::BitmapData imgData (img, juce::Image::BitmapData::readOnly);

    for (int y = 0; y < 400; y++)
    {
        uint8_t* p = imgData.getLinePointer (y);

        for (int x = 0; x < 640; x++)
        {
            uint32_t px = DG_ScreenBuffer[y * 640 + x];

            uint8_t b = (px & 0xff000000) >> 24;
            uint8_t r = (px & 0x00ff0000) >> 16;
            uint8_t g = (px & 0x0000ff00) >> 8;
            uint8_t a = (px & 0x000000ff) >> 0;

            juce::PixelARGB* s = (juce::PixelARGB*)p;
            s->setARGB (a, r, g, b);

            p += imgData.pixelStride;
        }
    }

    updateFrame (doom, img);
}

extern "C" void DG_SleepMs (data_t* /*data*/, uint32_t ms)
{
    juce::Thread::sleep (int (ms));
}

extern "C" uint32_t DG_GetTicksMs (data_t* /*data*/)
{
    static uint32_t startup = 0;
    if (startup == 0)
        startup = juce::Time::getMillisecondCounter();

    return juce::Time::getMillisecondCounter() - startup;
}

extern "C" int DG_GetKey (data_t* data, int* pressed, unsigned char* key)
{
	auto doom = (Doom*)data->user_data;

    auto event = getKeyEvent (doom);
    if (! event.has_value())
        return 0;

    *pressed = event->second ? 1 : 0;
    *key = (unsigned char)event->first;

    return 1;
}

extern "C" void DG_SetWindowTitle (data_t* data, const char* title)
{
    juce::ignoreUnused (data, title);
}

Doom::Doom()
    : juce::Thread ("Doom")
{
}

Doom::~Doom()
{
    runloop = 0;
    stopThread (100);
}

void Doom::registerComponent (DoomComponent* comp)
{
	juce::ScopedLock sl (lock);
	component = comp;
}

void Doom::startGame (juce::File wadFile_)
{
    wadFile = wadFile_;
    startThread();
}

juce::Image Doom::getScreen()
{
	juce::ScopedLock sl (lock);
	return screen;
}

void Doom::run()
{
	auto self = juce::WeakReference<Doom> (this);
    auto path = wadFile.getFullPathName();

	data_t* data;
	data = DG_Alloc();
	user_data = data;

	data->user_data = this;

    const char* params[3];
    params[0] = "doom";
    params[1] = "-iwad";
    params[2] = path.toRawUTF8();

    myargc = 3;
    myargv = (char**)params;

    M_FindResponseFile (data);

    // start doom
    printf("Starting D_DoomMain\r\n");

    dg_Create (data);

    D_DoomMain (data);

    juce::MessageManager::callAsync ([self]
    {
        if (self)
        {
            juce::ScopedLock sl (self->lock);
            self->screen = {};
			if (self->component)
				self->component->repaint();
        }
    });

	DG_Free (data);
}

void Doom::addEvent (int key, bool press)
{
    juce::ScopedLock sl (lock);
    keyEvents.push_back({key, press});
}

int Doom::mapKey (int key)
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

    return key;
}

