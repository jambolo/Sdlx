// Tests for Window, the owning SDL_Window wrapper. These run headless: the tests that need a window use SDL's dummy
// video driver, which creates windows without a display. If the driver isn't available, those tests are skipped.
//
// A test can tell whether a window was destroyed by looking up its ID: SDL_GetWindowFromID returns nullptr for a
// destroyed window.

#include <Sdlx/Window.h>

#include <gtest/gtest.h>

#include <cstring>
#include <type_traits>
#include <utility>

using Sdlx::Window;

static_assert(!std::is_copy_constructible_v<Window>, "Window must not be copyable");
static_assert(!std::is_copy_assignable_v<Window>, "Window must not be copyable");
static_assert(std::is_nothrow_move_constructible_v<Window>, "Window must be nothrow movable");
static_assert(std::is_nothrow_move_assignable_v<Window>, "Window must be nothrow movable");
static_assert(std::is_nothrow_swappable_v<Window>, "Window must be nothrow swappable");
static_assert(!std::is_convertible_v<SDL_Window *, Window>, "Taking ownership must be explicit");
static_assert(!std::is_convertible_v<SDL_PropertiesID, Window>, "Creating from properties must be explicit");
static_assert(!std::is_convertible_v<Window, bool>, "operator bool must be explicit");

// These need no video subsystem.

TEST(WindowTest, DefaultOwnsNothing)
{
    Window w;
    EXPECT_EQ(w.get(), nullptr);
    EXPECT_FALSE(w);
    EXPECT_EQ(w.release(), nullptr);
}

TEST(WindowTest, AccessorsFailWithoutAWindow)
{
    Window w;
    int width = 0;
    int height = 0;
    EXPECT_EQ(w.getID(), 0u);
    EXPECT_EQ(w.getFlags(), 0u);
    EXPECT_EQ(w.getProperties(), 0u);
    EXPECT_EQ(w.getParent(), nullptr);
    EXPECT_FALSE(w.setTitle("title"));
    EXPECT_FALSE(w.setSize(10, 10));
    EXPECT_FALSE(w.getSize(width, height));
    EXPECT_FALSE(w.show());
    EXPECT_EQ(w.getSurface(), nullptr);
    EXPECT_FALSE(w.hasSurface());
    EXPECT_TRUE(w.getICCProfile().empty());
    EXPECT_FALSE(w.createPopup(0, 0, 10, 10, SDL_WINDOW_TOOLTIP));

    // SDL_CreatePopupWindow would initialize the video subsystem
    EXPECT_EQ(SDL_WasInit(SDL_INIT_VIDEO), 0u);
}

TEST(WindowTest, DestroyingNothingKeepsTheError)
{
    SDL_SetError("unchanged");
    {
        Window w;
        w.reset();
    }
    EXPECT_STREQ(SDL_GetError(), "unchanged");
}

namespace
{

// Initializes the video subsystem with the dummy driver for each test
class WindowWithVideoTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            GTEST_SKIP() << "The dummy video driver is not available: " << SDL_GetError();
        }
        initialized_ = true;
    }

    void TearDown() override
    {
        if (initialized_)
        {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }
    }

    // Returns a new hidden window
    static SDL_Window *newWindow() { return SDL_CreateWindow("test", 64, 48, SDL_WINDOW_HIDDEN); }

    // Returns true if the window with the ID exists
    static bool exists(SDL_WindowID id) { return id != 0 && SDL_GetWindowFromID(id) != nullptr; }

  private:
    bool initialized_ = false;
};

} // namespace

TEST_F(WindowWithVideoTest, CreatesAWindow)
{
    Window w("title", 64, 48, SDL_WINDOW_HIDDEN);
    ASSERT_TRUE(w);
    EXPECT_STREQ(w.getTitle(), "title");
    int width = 0;
    int height = 0;
    EXPECT_TRUE(w.getSize(width, height));
    EXPECT_EQ(width, 64);
    EXPECT_EQ(height, 48);
    EXPECT_NE(w.getFlags() & SDL_WINDOW_HIDDEN, 0u);
}

TEST_F(WindowWithVideoTest, CreatesAWindowWithProperties)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    ASSERT_NE(props, 0u);
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "props");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 32);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 16);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    Window w(props);
    SDL_DestroyProperties(props);
    ASSERT_TRUE(w);
    EXPECT_STREQ(w.getTitle(), "props");
    int width = 0;
    int height = 0;
    EXPECT_TRUE(w.getSize(width, height));
    EXPECT_EQ(width, 32);
    EXPECT_EQ(height, 16);
}

TEST_F(WindowWithVideoTest, DestructorDestroysTheWindow)
{
    SDL_WindowID id = 0;
    {
        Window w(newWindow());
        ASSERT_TRUE(w);
        id = w.getID();
        EXPECT_TRUE(exists(id));
    }
    EXPECT_FALSE(exists(id));
}

TEST_F(WindowWithVideoTest, MoveConstructionTransfersOwnership)
{
    SDL_Window *raw = newWindow();
    ASSERT_NE(raw, nullptr);
    SDL_WindowID id = SDL_GetWindowID(raw);
    {
        Window a(raw);
        Window b(std::move(a));
        EXPECT_EQ(a.get(), nullptr); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        EXPECT_EQ(b.get(), raw);
        EXPECT_TRUE(exists(id));
    }
    EXPECT_FALSE(exists(id));
}

TEST_F(WindowWithVideoTest, MoveAssignmentDestroysTheOldWindow)
{
    SDL_Window *first = newWindow();
    SDL_Window *second = newWindow();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    SDL_WindowID firstID = SDL_GetWindowID(first);
    SDL_WindowID secondID = SDL_GetWindowID(second);
    {
        Window a(first);
        Window b(second);
        a = std::move(b);
        EXPECT_FALSE(exists(firstID));
        EXPECT_EQ(a.get(), second);
        EXPECT_EQ(b.get(), nullptr); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        EXPECT_TRUE(exists(secondID));
    }
    EXPECT_FALSE(exists(secondID));
}

TEST_F(WindowWithVideoTest, SelfMoveAssignmentKeepsTheWindow)
{
    SDL_Window *raw = newWindow();
    ASSERT_NE(raw, nullptr);
    SDL_WindowID id = SDL_GetWindowID(raw);
    Window w(raw);
    Window &alias = w;
    w = std::move(alias);
    EXPECT_EQ(w.get(), raw);
    EXPECT_TRUE(exists(id));
}

TEST_F(WindowWithVideoTest, ReleaseGivesUpOwnership)
{
    SDL_Window *raw = newWindow();
    ASSERT_NE(raw, nullptr);
    SDL_WindowID id = SDL_GetWindowID(raw);
    {
        Window w(raw);
        EXPECT_EQ(w.release(), raw);
        EXPECT_EQ(w.get(), nullptr);
    }
    EXPECT_TRUE(exists(id));
    SDL_DestroyWindow(raw);
}

TEST_F(WindowWithVideoTest, ResetDestroysTheOldWindow)
{
    SDL_Window *first = newWindow();
    SDL_Window *second = newWindow();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    SDL_WindowID firstID = SDL_GetWindowID(first);
    SDL_WindowID secondID = SDL_GetWindowID(second);
    Window w(first);
    w.reset(second);
    EXPECT_FALSE(exists(firstID));
    EXPECT_EQ(w.get(), second);
    w.reset();
    EXPECT_FALSE(exists(secondID));
    EXPECT_EQ(w.get(), nullptr);
}

TEST_F(WindowWithVideoTest, ResetToTheSameWindowKeepsIt)
{
    SDL_Window *raw = newWindow();
    ASSERT_NE(raw, nullptr);
    SDL_WindowID id = SDL_GetWindowID(raw);
    Window w(raw);
    w.reset(raw);
    EXPECT_EQ(w.get(), raw);
    EXPECT_TRUE(exists(id));
}

TEST_F(WindowWithVideoTest, SwapExchangesWindows)
{
    SDL_Window *first = newWindow();
    SDL_Window *second = newWindow();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    Window a(first);
    Window b(second);
    a.swap(b);
    EXPECT_EQ(a.get(), second);
    EXPECT_EQ(b.get(), first);
    using std::swap;
    swap(a, b);
    EXPECT_EQ(a.get(), first);
    EXPECT_EQ(b.get(), second);
}

TEST_F(WindowWithVideoTest, Title)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    EXPECT_TRUE(w.setTitle("new title"));
    EXPECT_STREQ(w.getTitle(), "new title");
}

TEST_F(WindowWithVideoTest, Size)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    EXPECT_TRUE(w.setSize(100, 80));
    EXPECT_TRUE(w.sync());
    int width = 0;
    int height = 0;
    EXPECT_TRUE(w.getSize(width, height));
    EXPECT_EQ(width, 100);
    EXPECT_EQ(height, 80);
    EXPECT_TRUE(w.getSizeInPixels(width, height));
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

TEST_F(WindowWithVideoTest, MinimumAndMaximumSize)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    int width = 0;
    int height = 0;
    EXPECT_TRUE(w.setMinimumSize(20, 10));
    EXPECT_TRUE(w.getMinimumSize(width, height));
    EXPECT_EQ(width, 20);
    EXPECT_EQ(height, 10);
    EXPECT_TRUE(w.setMaximumSize(200, 100));
    EXPECT_TRUE(w.getMaximumSize(width, height));
    EXPECT_EQ(width, 200);
    EXPECT_EQ(height, 100);
}

TEST_F(WindowWithVideoTest, AspectRatio)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    float minAspect = 0.0f;
    float maxAspect = 0.0f;
    EXPECT_TRUE(w.setAspectRatio(1.0f, 2.0f));
    EXPECT_TRUE(w.getAspectRatio(minAspect, maxAspect));
    EXPECT_FLOAT_EQ(minAspect, 1.0f);
    EXPECT_FLOAT_EQ(maxAspect, 2.0f);
}

TEST_F(WindowWithVideoTest, ShowAndHide)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    EXPECT_TRUE(w.show());
    EXPECT_EQ(w.getFlags() & SDL_WINDOW_HIDDEN, 0u);
    EXPECT_TRUE(w.hide());
    EXPECT_NE(w.getFlags() & SDL_WINDOW_HIDDEN, 0u);
}

TEST_F(WindowWithVideoTest, Properties)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    EXPECT_NE(w.getProperties(), 0u);
}

TEST_F(WindowWithVideoTest, Surface)
{
    Window w(newWindow());
    ASSERT_TRUE(w);
    EXPECT_FALSE(w.hasSurface());
    SDL_Surface *surface = w.getSurface();
    ASSERT_NE(surface, nullptr);
    EXPECT_TRUE(w.hasSurface());
    EXPECT_EQ(surface->w, 64);
    EXPECT_EQ(surface->h, 48);
    EXPECT_TRUE(SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGB(surface, 255, 0, 0)));
    EXPECT_TRUE(w.updateSurface());
    SDL_Rect rect{0, 0, 8, 8};
    EXPECT_TRUE(w.updateSurfaceRects(&rect, 1));
    EXPECT_TRUE(w.destroySurface());
    EXPECT_FALSE(w.hasSurface());
}

