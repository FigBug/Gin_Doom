
extern "C"
{
#include "../../../doomgeneric/m_argv.h"

void D_DoomMain (void);
void M_FindResponseFile(void);
void dg_Create();

extern uint32_t* DG_ScreenBuffer;
}

static DoomComponent* dc = nullptr;

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

    juce::ScopedLock sl (dc->lock);
    dc->screen = img;
    juce::MessageManager::callAsync ([]
    {
        if (dc)
            dc->repaint();
    });
}

extern "C" void DG_SleepMs(uint32_t ms)
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

extern "C" int DG_GetKey(int* pressed, unsigned char* key)
{
    juce::ignoreUnused (pressed, key);
    return 0;
}

extern "C" void DG_SetWindowTitle(const char* title)
{
    juce::ignoreUnused (title);
}

DoomComponent::DoomComponent()
    : juce::Thread ("Doom")
{
    setOpaque (true);
    startThread();
}

void DoomComponent::run()
{
    dc = this;

    const char* params[3];
    params[0] = "doom";
    params[1] = "-iwad";
    params[2] = "/Users/rrabien/Downloads/DOOM1.WAD";

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
