// Tests for the helpers in Sdlx.h and for Sprite::Draw. These run headless:
// they use in-memory surfaces and the SDL event queue, and never create a
// window.

#include <Sdlx/Sdlx.h>
#include <Sdlx/Sprite.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{

struct Pixel
{
    Uint8 r, g, b;
    bool operator==(Pixel const &o) const
    {
        return r == o.r && g == o.g && b == o.b;
    }
};

std::ostream &operator<<(std::ostream &os, Pixel const &p)
{
    return os << "(" << int(p.r) << "," << int(p.g) << "," << int(p.b) << ")";
}

Pixel const BLACK = {0, 0, 0};
Pixel const RED = {255, 0, 0};
Pixel const GREEN = {0, 255, 0};
Pixel const BLUE = {0, 0, 255};
Pixel const MAGENTA = {255, 0, 255};

SDL_Surface *NewSurface(int w, int h, Pixel fill)
{
    SDL_Surface *s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (s != nullptr)
    {
        SDL_FillSurfaceRect(s, nullptr, SDL_MapSurfaceRGB(s, fill.r, fill.g, fill.b));
    }
    return s;
}

void Fill(SDL_Surface *s, SDL_Rect rect, Pixel p)
{
    SDL_FillSurfaceRect(s, &rect, SDL_MapSurfaceRGB(s, p.r, p.g, p.b));
}

Pixel At(SDL_Surface *s, int x, int y)
{
    Pixel p = {0, 0, 0};
    Uint8 a;
    EXPECT_TRUE(SDL_ReadSurfacePixel(s, x, y, &p.r, &p.g, &p.b, &a)) << SDL_GetError();
    return p;
}

// Owns a surface for the duration of a test
struct SurfacePtr
{
    explicit SurfacePtr(SDL_Surface *s = nullptr) : surface(s)
    {
    }
    ~SurfacePtr()
    {
        SDL_DestroySurface(surface);
    }
    SurfacePtr(SurfacePtr const &) = delete;
    SurfacePtr &operator=(SurfacePtr const &) = delete;
    SDL_Surface *get() const
    {
        return surface;
    }
    SDL_Surface *surface;
};

// A temporary file path that is removed when the object goes out of scope
struct TempFile
{
    explicit TempFile(std::string const &name) : path((std::filesystem::temp_directory_path() / name).string())
    {
    }
    ~TempFile()
    {
        std::remove(path.c_str());
    }
    std::string path;
};

// A 3x3 blue 8-bit RGB PNG file. It is embedded so that the tests need no PNG
// writer (SDL_SavePNG requires SDL 3.4).
unsigned char const BLUE_3X3_PNG[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00,
    0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x08, 0x02, 0x00, 0x00, 0x00, 0xd9, 0x4a, 0x22, 0xe8, 0x00, 0x00, 0x00,
    0x0f, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x60, 0x60, 0xf8, 0x0f, 0x43, 0x98, 0x2c, 0x00, 0x7d, 0xa0,
    0x08, 0xf8, 0xa6, 0x20, 0x7f, 0x64, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

bool WriteFile(std::string const &path, unsigned char const *data, size_t size)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<char const *>(data), std::streamsize(size));
    return file.good();
}

// Saves an 8-bit paletted BMP that is one pixel high and has one pixel for each
// palette entry: pixel i has the color palette[i].
bool SavePalettedBmp(std::string const &path, std::vector<SDL_Color> const &palette)
{
    SurfacePtr surface(SDL_CreateSurface(int(palette.size()), 1, SDL_PIXELFORMAT_INDEX8));
    if (surface.get() == nullptr)
    {
        return false;
    }

    SDL_Palette *p = SDL_CreateSurfacePalette(surface.get());
    if (p == nullptr || !SDL_SetPaletteColors(p, palette.data(), 0, int(palette.size())))
    {
        return false;
    }

    Uint8 *pixels = static_cast<Uint8 *>(surface.get()->pixels);
    for (size_t i = 0; i < palette.size(); ++i)
    {
        pixels[i] = Uint8(i);
    }

    return SDL_SaveBMP(surface.get(), path.c_str());
}

} // anonymous namespace

/********************************************************************************************************************/
/* Helpers */
/********************************************************************************************************************/

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

TEST(HelpersTest, MakeRectSetsFields)
{
    SDL_Rect r = Sdlx::MakeRect(1, 2, 3, 4);

    EXPECT_EQ(r.x, 1);
    EXPECT_EQ(r.y, 2);
    EXPECT_EQ(r.w, 3);
    EXPECT_EQ(r.h, 4);
}

TEST(HelpersTest, MakeColorSetsFieldsAndIsOpaque)
{
    SDL_Color c = Sdlx::MakeColor(10, 20, 30);

    EXPECT_EQ(c.r, 10);
    EXPECT_EQ(c.g, 20);
    EXPECT_EQ(c.b, 30);
    EXPECT_EQ(c.a, 255);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/********************************************************************************************************************/
/* Image loading */
/********************************************************************************************************************/

TEST(LoadSurfaceTest, LoadsBmp)
{
    SurfacePtr source(NewSurface(4, 2, RED));
    Fill(source.get(), SDL_Rect{2, 0, 2, 2}, GREEN);
    TempFile file("sdlx_test_load.bmp");
    ASSERT_TRUE(SDL_SaveBMP(source.get(), file.path.c_str())) << SDL_GetError();

    SurfacePtr image(Sdlx::LoadSurface(file.path.c_str()));

    ASSERT_NE(image.get(), nullptr) << SDL_GetError();
    EXPECT_EQ(image.get()->w, 4);
    EXPECT_EQ(image.get()->h, 2);
    EXPECT_EQ(At(image.get(), 0, 0), RED);
    EXPECT_EQ(At(image.get(), 3, 1), GREEN);
}

TEST(LoadSurfaceTest, LoadsPng)
{
    TempFile file("sdlx_test_load.png");
    ASSERT_TRUE(WriteFile(file.path, BLUE_3X3_PNG, sizeof(BLUE_3X3_PNG)));

    SurfacePtr image(Sdlx::LoadSurface(file.path.c_str()));

    ASSERT_NE(image.get(), nullptr) << SDL_GetError();
    EXPECT_EQ(image.get()->w, 3);
    EXPECT_EQ(image.get()->h, 3);
    EXPECT_EQ(At(image.get(), 1, 1), BLUE);
}

TEST(LoadSurfaceTest, MissingFileReturnsNull)
{
    EXPECT_EQ(Sdlx::LoadSurface("this/file/does/not/exist.png"), nullptr);
}

TEST(LoadColorKeyedSurfaceTest, SetsColorKey)
{
    SurfacePtr source(NewSurface(2, 1, MAGENTA));
    TempFile file("sdlx_test_key.bmp");
    ASSERT_TRUE(SDL_SaveBMP(source.get(), file.path.c_str())) << SDL_GetError();

    SurfacePtr image(Sdlx::LoadColorKeyedSurface(file.path.c_str(), SDL_Color{255, 0, 255, 255}));

    ASSERT_NE(image.get(), nullptr) << SDL_GetError();
    EXPECT_TRUE(SDL_SurfaceHasColorKey(image.get()));

    Uint32 key = 0;
    ASSERT_TRUE(SDL_GetSurfaceColorKey(image.get(), &key));
    EXPECT_EQ(key, SDL_MapSurfaceRGB(image.get(), 255, 0, 255));
}

TEST(LoadColorKeyedSurfaceTest, KeyedPixelsAreNotBlitted)
{
    SurfacePtr source(NewSurface(2, 1, MAGENTA));
    Fill(source.get(), SDL_Rect{1, 0, 1, 1}, RED);
    TempFile file("sdlx_test_keyblit.bmp");
    ASSERT_TRUE(SDL_SaveBMP(source.get(), file.path.c_str())) << SDL_GetError();

    SurfacePtr image(Sdlx::LoadColorKeyedSurface(file.path.c_str(), SDL_Color{255, 0, 255, 255}));
    ASSERT_NE(image.get(), nullptr) << SDL_GetError();

    SurfacePtr dst(NewSurface(2, 1, BLUE));
    ASSERT_TRUE(SDL_BlitSurface(image.get(), nullptr, dst.get(), nullptr)) << SDL_GetError();

    EXPECT_EQ(At(dst.get(), 0, 0), BLUE); // the key color is transparent
    EXPECT_EQ(At(dst.get(), 1, 0), RED);
}

TEST(LoadColorKeyedSurfaceTest, PalettedImageIsConvertedToTrueColor)
{
    TempFile file("sdlx_test_palette.bmp");
    ASSERT_TRUE(SavePalettedBmp(file.path, {SDL_Color{255, 0, 255, 255}, SDL_Color{255, 0, 0, 255}})) << SDL_GetError();

    // Check that the file really is loaded as a paletted surface, or the other
    // paletted tests would prove nothing.
    SurfacePtr plain(Sdlx::LoadSurface(file.path.c_str()));
    ASSERT_NE(plain.get(), nullptr) << SDL_GetError();
    ASSERT_TRUE(SDL_ISPIXELFORMAT_INDEXED(plain.get()->format));

    SurfacePtr image(Sdlx::LoadColorKeyedSurface(file.path.c_str(), SDL_Color{255, 0, 255, 255}));

    ASSERT_NE(image.get(), nullptr) << SDL_GetError();
    EXPECT_FALSE(SDL_ISPIXELFORMAT_INDEXED(image.get()->format));
    EXPECT_TRUE(SDL_SurfaceHasColorKey(image.get()));
    EXPECT_EQ(image.get()->w, 2);
    EXPECT_EQ(image.get()->h, 1);
}

TEST(LoadColorKeyedSurfaceTest, PalettedImageWithoutTheKeyColorHasNoTransparentPixels)
{
    // Pink is the palette entry nearest to the magenta key, but it is not the key
    Pixel const PINK = {255, 0, 200};
    TempFile file("sdlx_test_palette_nokey.bmp");
    ASSERT_TRUE(SavePalettedBmp(file.path, {SDL_Color{255, 0, 200, 255}, SDL_Color{255, 0, 0, 255}})) << SDL_GetError();

    SurfacePtr image(Sdlx::LoadColorKeyedSurface(file.path.c_str(), SDL_Color{255, 0, 255, 255}));
    ASSERT_NE(image.get(), nullptr) << SDL_GetError();

    SurfacePtr dst(NewSurface(2, 1, BLUE));
    ASSERT_TRUE(SDL_BlitSurface(image.get(), nullptr, dst.get(), nullptr)) << SDL_GetError();

    EXPECT_EQ(At(dst.get(), 0, 0), PINK);
    EXPECT_EQ(At(dst.get(), 1, 0), RED);
}

TEST(LoadColorKeyedSurfaceTest, EveryPaletteEntryOfTheKeyColorIsTransparent)
{
    TempFile file("sdlx_test_palette_twice.bmp");
    ASSERT_TRUE(SavePalettedBmp(file.path,
                                {SDL_Color{255, 0, 255, 255}, SDL_Color{255, 0, 0, 255}, SDL_Color{255, 0, 255, 255}}))
        << SDL_GetError();

    SurfacePtr image(Sdlx::LoadColorKeyedSurface(file.path.c_str(), SDL_Color{255, 0, 255, 255}));
    ASSERT_NE(image.get(), nullptr) << SDL_GetError();

    SurfacePtr dst(NewSurface(3, 1, BLUE));
    ASSERT_TRUE(SDL_BlitSurface(image.get(), nullptr, dst.get(), nullptr)) << SDL_GetError();

    EXPECT_EQ(At(dst.get(), 0, 0), BLUE);
    EXPECT_EQ(At(dst.get(), 1, 0), RED);
    EXPECT_EQ(At(dst.get(), 2, 0), BLUE);
}

TEST(LoadColorKeyedSurfaceTest, MissingFileReturnsNull)
{
    EXPECT_EQ(Sdlx::LoadColorKeyedSurface("this/file/does/not/exist.png", SDL_Color{0, 0, 0, 255}), nullptr);
}

/********************************************************************************************************************/
/* Sprite::Draw */
/********************************************************************************************************************/

namespace
{

// An 8x8 sheet: the left half is red and the right half is green
class SpriteDrawTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        sheet.reset(new SurfacePtr(NewSurface(8, 8, RED)));
        Fill(sheet->get(), SDL_Rect{4, 0, 4, 8}, GREEN);
        dst.reset(new SurfacePtr(NewSurface(32, 32, BLACK)));
    }

    std::unique_ptr<SurfacePtr> sheet;
    std::unique_ptr<SurfacePtr> dst;
};

} // anonymous namespace

TEST_F(SpriteDrawTest, DrawsSubRegionAtPositionMinusOffset)
{
    Sdlx::Sprite sprite(sheet->get(), SDL_Rect{4, 0, 4, 8}, 1, 1, 10.0f, 10.0f);
    sprite.Draw(dst->get());

    // Drawn at (9, 9), size 4x8
    EXPECT_EQ(At(dst->get(), 9, 9), GREEN);
    EXPECT_EQ(At(dst->get(), 12, 16), GREEN);
    EXPECT_EQ(At(dst->get(), 8, 9), BLACK);
    EXPECT_EQ(At(dst->get(), 13, 9), BLACK);
    EXPECT_EQ(At(dst->get(), 9, 8), BLACK);
    EXPECT_EQ(At(dst->get(), 9, 17), BLACK);
}

TEST_F(SpriteDrawTest, EmptyRectDrawsWholeSheet)
{
    Sdlx::Sprite sprite(sheet->get(), SDL_Rect{0, 0, 0, 0}, 0, 0, 2.0f, 3.0f);
    sprite.Draw(dst->get());

    EXPECT_EQ(At(dst->get(), 2, 3), RED);
    EXPECT_EQ(At(dst->get(), 5, 3), RED);
    EXPECT_EQ(At(dst->get(), 6, 3), GREEN);
    EXPECT_EQ(At(dst->get(), 9, 10), GREEN);
    EXPECT_EQ(At(dst->get(), 10, 10), BLACK);
}

TEST_F(SpriteDrawTest, PositionIsRoundedToNearestPixel)
{
    Sdlx::Sprite down(sheet->get(), SDL_Rect{0, 0, 0, 0}, 0, 0, 10.4f, 10.4f);
    down.Draw(dst->get());
    EXPECT_EQ(At(dst->get(), 10, 10), RED);
    EXPECT_EQ(At(dst->get(), 9, 10), BLACK);

    SurfacePtr other(NewSurface(32, 32, BLACK));
    Sdlx::Sprite up(sheet->get(), SDL_Rect{0, 0, 0, 0}, 0, 0, 10.6f, 10.6f);
    up.Draw(other.get());
    EXPECT_EQ(At(other.get(), 11, 11), RED);
    EXPECT_EQ(At(other.get(), 10, 10), BLACK);
}

TEST_F(SpriteDrawTest, NegativePositionIsRoundedToNearestPixel)
{
    // -1.2 rounds to -1, so the first column and row of the sheet are clipped
    Sdlx::Sprite up(sheet->get(), SDL_Rect{0, 0, 0, 0}, 0, 0, -1.2f, -1.2f);
    up.Draw(dst->get());
    EXPECT_EQ(At(dst->get(), 2, 0), RED);
    EXPECT_EQ(At(dst->get(), 3, 0), GREEN);
    EXPECT_EQ(At(dst->get(), 6, 6), GREEN);
    EXPECT_EQ(At(dst->get(), 7, 0), BLACK);
    EXPECT_EQ(At(dst->get(), 0, 7), BLACK);

    // -1.6 rounds to -2
    SurfacePtr other(NewSurface(32, 32, BLACK));
    Sdlx::Sprite down(sheet->get(), SDL_Rect{0, 0, 0, 0}, 0, 0, -1.6f, -1.6f);
    down.Draw(other.get());
    EXPECT_EQ(At(other.get(), 1, 0), RED);
    EXPECT_EQ(At(other.get(), 2, 0), GREEN);
    EXPECT_EQ(At(other.get(), 5, 5), GREEN);
    EXPECT_EQ(At(other.get(), 6, 0), BLACK);
    EXPECT_EQ(At(other.get(), 0, 6), BLACK);
}

TEST_F(SpriteDrawTest, NewAnimatedSpriteDrawsItsFirstFrameNotTheWholeSheet)
{
    Sdlx::AnimatedSprite::AnimationGroup group;
    group.images.push_back({SDL_Rect{0, 0, 4, 8}, 0, 0}); // red
    group.images.push_back({SDL_Rect{4, 0, 4, 8}, 0, 0}); // green
    Sdlx::AnimatedSprite::Animation animation;
    animation.frames = {{0, 1.0f}, {1, 1.0f}};
    group.animations.push_back(animation);

    // Drawn without calling PlayAnimation
    Sdlx::AnimatedSprite sprite(sheet->get(), &group, 4.0f, 4.0f);
    sprite.Draw(dst->get());

    EXPECT_EQ(At(dst->get(), 4, 4), RED);
    EXPECT_EQ(At(dst->get(), 7, 4), RED);
    EXPECT_EQ(At(dst->get(), 8, 4), BLACK); // the green half of the sheet is not drawn
}

TEST_F(SpriteDrawTest, AnimatedSpriteWithoutAnImageDrawsNothing)
{
    // A plain Sprite with an empty rectangle draws the whole sheet. An animated
    // sprite with no current image must not.
    Sdlx::AnimatedSprite::AnimationGroup empty;
    Sdlx::AnimatedSprite sprite(sheet->get(), &empty, 4.0f, 4.0f);
    sprite.Draw(dst->get());

    EXPECT_EQ(At(dst->get(), 4, 4), BLACK);
    EXPECT_EQ(At(dst->get(), 8, 4), BLACK);

    // The same through a pointer to the base class
    Sdlx::Sprite const &base = sprite;
    base.Draw(dst->get());

    EXPECT_EQ(At(dst->get(), 4, 4), BLACK);
}

TEST_F(SpriteDrawTest, AnimatedSpriteDrawsCurrentFrame)
{
    Sdlx::AnimatedSprite::AnimationGroup group;
    group.images.push_back({SDL_Rect{0, 0, 4, 8}, 0, 0}); // red
    group.images.push_back({SDL_Rect{4, 0, 4, 8}, 0, 0}); // green
    Sdlx::AnimatedSprite::Animation animation;
    animation.mode = Sdlx::AnimatedSprite::Animation::MODE_LOOP;
    animation.frames = {{0, 1.0f}, {1, 1.0f}};
    group.animations.push_back(animation);

    Sdlx::AnimatedSprite sprite(sheet->get(), &group, 4.0f, 4.0f);
    sprite.PlayAnimation(0);
    sprite.Draw(dst->get());
    EXPECT_EQ(At(dst->get(), 4, 4), RED);

    sprite.AdvanceTime(1.0f);
    sprite.Draw(dst->get());
    EXPECT_EQ(At(dst->get(), 4, 4), GREEN);
}

/********************************************************************************************************************/
/* EventLoop */
/********************************************************************************************************************/

namespace
{

int g_events;
int g_idleCalls;
int g_stopAfter; // handler returns false after this many events (0 = until
                 // quit)

void PushEvent(Uint32 type)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = type;
    ASSERT_TRUE(SDL_PushEvent(&e)) << SDL_GetError();
}

bool CountingHandler(SDL_Event const &event)
{
    ++g_events;
    if (g_stopAfter > 0 && g_events >= g_stopAfter)
    {
        return false;
    }
    return event.type != SDL_EVENT_QUIT;
}

bool IdleThenQuit()
{
    ++g_idleCalls;
    if (g_idleCalls == 3)
    {
        PushEvent(SDL_EVENT_QUIT);
    }
    return true;
}

std::thread g_pusher; // pushes an event from another thread, after a delay

// Always asks not to be called again until after the next event. The first call
// arranges for an event to arrive later, and the first call after that event
// has been handled ends the loop.
bool IdleUntilAnEventIsHandled()
{
    ++g_idleCalls;
    if (g_idleCalls == 1)
    {
        g_pusher = std::thread([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SDL_Event e;
            SDL_zero(e);
            e.type = SDL_EVENT_USER;
            SDL_PushEvent(&e);
        });
    }
    if (g_events > 0)
    {
        PushEvent(SDL_EVENT_QUIT);
    }
    return false;
}

class EventLoopTest : public ::testing::Test
{
  protected:
    static void SetUpTestSuite()
    {
        ASSERT_TRUE(SDL_Init(SDL_INIT_EVENTS)) << SDL_GetError();
    }
    static void TearDownTestSuite()
    {
        SDL_Quit();
    }

    void SetUp() override
    {
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
        g_events = 0;
        g_idleCalls = 0;
        g_stopAfter = 0;
    }
};

} // anonymous namespace

TEST_F(EventLoopTest, DefaultHandlerReturnsOnQuit)
{
    PushEvent(SDL_EVENT_QUIT);

    // This would hang if the quit event were not recognized
    EXPECT_TRUE(Sdlx::EventLoop());
}

// This test is not in the EventLoopTest suite, because it needs the SDL events
// subsystem to be shut down.
TEST(EventLoopWithoutEventsTest, ReturnsFalseWhenWaitingFails)
{
    ASSERT_EQ(SDL_WasInit(SDL_INIT_EVENTS), 0u);

    // Waiting for an event fails when the events subsystem is not initialized.
    // The loop must report that instead of waiting forever.
    EXPECT_FALSE(Sdlx::EventLoop());
}

TEST(EventLoopWithoutEventsTest, ReturnsFalseWithAnIdleCallbackThatAlwaysWantsToBeCalled)
{
    ASSERT_EQ(SDL_WasInit(SDL_INIT_EVENTS), 0u);

    // With an idle callback, the loop polls instead of waiting. No event can
    // arrive, so it must return instead of calling the callback forever.
    g_idleCalls = 0;
    EXPECT_FALSE(Sdlx::EventLoop(nullptr, [] {
        ++g_idleCalls;
        return true;
    }));
    EXPECT_EQ(g_idleCalls, 0);
}

TEST_F(EventLoopTest, HandlerSeesEveryEventUntilQuit)
{
    PushEvent(SDL_EVENT_USER);
    PushEvent(SDL_EVENT_USER);
    PushEvent(SDL_EVENT_USER);
    PushEvent(SDL_EVENT_QUIT);

    Sdlx::EventLoop(CountingHandler);

    EXPECT_EQ(g_events, 4);
}

TEST_F(EventLoopTest, HandlerReturningFalseStopsTheLoop)
{
    g_stopAfter = 2;
    PushEvent(SDL_EVENT_USER);
    PushEvent(SDL_EVENT_USER);
    PushEvent(SDL_EVENT_USER);

    Sdlx::EventLoop(CountingHandler);

    EXPECT_EQ(g_events, 2);
}

TEST_F(EventLoopTest, IdleCallbackRunsWhenThereAreNoEvents)
{
    Sdlx::EventLoop(nullptr, IdleThenQuit);

    EXPECT_EQ(g_idleCalls, 3);
}

TEST_F(EventLoopTest, IdleCallbackReturningFalseIsNotCalledAgainUntilNextEvent)
{
    // After the first idle call returns false, the loop must block until the
    // event from the other thread arrives. If it kept calling the idle callback
    // instead, the callback would be called many times during the delay.
    Sdlx::EventLoop(CountingHandler, IdleUntilAnEventIsHandled);
    g_pusher.join();

    EXPECT_EQ(g_idleCalls, 2);
    EXPECT_EQ(g_events, 2); // the delayed event and the quit event
}
