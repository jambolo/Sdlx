// Tests for Surface, the owning SDL_Surface wrapper. These run headless: they use in-memory surfaces only.
//
// An SDL_Surface has a reference count. A test holds an extra reference to the surface it wraps, so that it can tell
// whether the wrapper released its own reference: the count drops back to 1.

#include <Sdlx/Surface.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>
#include <utility>

using Sdlx::Surface;

namespace
{

SDL_Surface *newSurface()
{
    return SDL_CreateSurface(4, 4, SDL_PIXELFORMAT_RGBA32);
}

// Returns a new surface with an extra reference, so that it survives the wrapper that owns it
SDL_Surface *newTrackedSurface()
{
    SDL_Surface *s = newSurface();
    if (s != nullptr)
    {
        ++s->refcount;
    }
    return s;
}

} // namespace

static_assert(std::is_nothrow_copy_constructible_v<Surface>, "Surface must be nothrow copyable");
static_assert(std::is_nothrow_copy_assignable_v<Surface>, "Surface must be nothrow copyable");
static_assert(std::is_nothrow_move_constructible_v<Surface>, "Surface must be nothrow movable");
static_assert(std::is_nothrow_move_assignable_v<Surface>, "Surface must be nothrow movable");
static_assert(std::is_nothrow_swappable_v<Surface>, "Surface must be nothrow swappable");
static_assert(!std::is_convertible_v<SDL_Surface *, Surface>, "Taking ownership must be explicit");
static_assert(!std::is_convertible_v<Surface, bool>, "operator bool must be explicit");

TEST(SurfaceTest, DefaultOwnsNothing)
{
    Surface s;
    EXPECT_EQ(s.get(), nullptr);
    EXPECT_FALSE(s);
}

TEST(SurfaceTest, OwnsTheSurface)
{
    SDL_Surface *raw = newSurface();
    ASSERT_NE(raw, nullptr);
    Surface s(raw);
    EXPECT_EQ(s.get(), raw);
    EXPECT_TRUE(s);
    EXPECT_EQ(s->w, 4);
    EXPECT_EQ(s->h, 4);
}

TEST(SurfaceTest, DestructorDestroysTheSurface)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface s(raw);
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, MoveConstructionTransfersOwnership)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface a(raw);
        Surface b(std::move(a));
        EXPECT_EQ(a.get(), nullptr); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        EXPECT_EQ(b.get(), raw);
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, MoveAssignmentDestroysTheOldSurface)
{
    SDL_Surface *first = newTrackedSurface();
    SDL_Surface *second = newTrackedSurface();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    {
        Surface a(first);
        Surface b(second);
        a = std::move(b);
        EXPECT_EQ(first->refcount, 1);
        EXPECT_EQ(a.get(), second);
        EXPECT_EQ(b.get(), nullptr); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    }
    EXPECT_EQ(second->refcount, 1);
    SDL_DestroySurface(first);
    SDL_DestroySurface(second);
}

TEST(SurfaceTest, SelfMoveAssignmentKeepsTheSurface)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface s(raw);
        Surface &alias = s;
        s = std::move(alias);
        EXPECT_EQ(s.get(), raw);
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, ReleaseGivesUpOwnership)
{
    SDL_Surface *raw = newSurface();
    ASSERT_NE(raw, nullptr);
    SDL_Surface *released = nullptr;
    {
        Surface s(raw);
        released = s.release();
        EXPECT_EQ(s.get(), nullptr);
    }
    EXPECT_EQ(released, raw);
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, ResetDestroysTheOldSurface)
{
    SDL_Surface *first = newTrackedSurface();
    SDL_Surface *second = newTrackedSurface();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    {
        Surface s(first);
        s.reset(second);
        EXPECT_EQ(first->refcount, 1);
        EXPECT_EQ(s.get(), second);
        s.reset();
        EXPECT_EQ(second->refcount, 1);
        EXPECT_FALSE(s);
    }
    SDL_DestroySurface(first);
    SDL_DestroySurface(second);
}

TEST(SurfaceTest, ResetToTheSameSurfaceKeepsIt)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface s(raw);
        s.reset(raw);
        EXPECT_EQ(s.get(), raw);
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, SwapExchangesSurfaces)
{
    SDL_Surface *first = newSurface();
    SDL_Surface *second = newSurface();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    Surface a(first);
    Surface b(second);
    a.swap(b);
    EXPECT_EQ(a.get(), second);
    EXPECT_EQ(b.get(), first);
    using std::swap;
    swap(a, b);
    EXPECT_EQ(a.get(), first);
    EXPECT_EQ(b.get(), second);
}

TEST(SurfaceTest, ColorKey)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    Uint32 key = 0;
    EXPECT_FALSE(s.hasColorKey());
    EXPECT_FALSE(s.getColorKey(key));

    Uint32 const magenta = SDL_MapSurfaceRGB(s.get(), 255, 0, 255);
    EXPECT_TRUE(s.setColorKey(true, magenta));
    EXPECT_TRUE(s.hasColorKey());
    EXPECT_TRUE(s.getColorKey(key));
    EXPECT_EQ(key, magenta);

    EXPECT_TRUE(s.setColorKey(false, 0));
    EXPECT_FALSE(s.hasColorKey());
}

TEST(SurfaceTest, ColorAndAlphaMod)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.setColorMod(10, 20, 30));
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    EXPECT_TRUE(s.getColorMod(r, g, b));
    EXPECT_EQ(r, 10);
    EXPECT_EQ(g, 20);
    EXPECT_EQ(b, 30);

    EXPECT_TRUE(s.setAlphaMod(40));
    Uint8 alpha = 0;
    EXPECT_TRUE(s.getAlphaMod(alpha));
    EXPECT_EQ(alpha, 40);
}

TEST(SurfaceTest, BlendMode)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.setBlendMode(SDL_BLENDMODE_ADD));
    SDL_BlendMode mode = SDL_BLENDMODE_NONE;
    EXPECT_TRUE(s.getBlendMode(mode));
    EXPECT_EQ(mode, SDL_BLENDMODE_ADD);
}

TEST(SurfaceTest, ClipRect)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    SDL_Rect const clip = {1, 1, 2, 2};
    EXPECT_TRUE(s.setClipRect(&clip));
    SDL_Rect rect = {0, 0, 0, 0};
    EXPECT_TRUE(s.getClipRect(rect));
    EXPECT_TRUE(SDL_RectsEqual(&rect, &clip));

    EXPECT_TRUE(s.setClipRect(nullptr));
    EXPECT_TRUE(s.getClipRect(rect));
    SDL_Rect const whole = {0, 0, 4, 4};
    EXPECT_TRUE(SDL_RectsEqual(&rect, &whole));
}

TEST(SurfaceTest, RLE)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.setRLE(true));
    EXPECT_TRUE(s.hasRLE());
    EXPECT_TRUE(s.setRLE(false));
    EXPECT_FALSE(s.hasRLE());
}

TEST(SurfaceTest, Colorspace)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.setColorspace(SDL_COLORSPACE_SRGB_LINEAR));
    EXPECT_EQ(s.getColorspace(), SDL_COLORSPACE_SRGB_LINEAR);
}

TEST(SurfaceTest, Palette)
{
    Surface s(SDL_CreateSurface(4, 4, SDL_PIXELFORMAT_INDEX8));
    ASSERT_TRUE(s);
    SDL_Palette *created = s.createPalette();
    ASSERT_NE(created, nullptr);
    EXPECT_EQ(s.getPalette(), created);

    SDL_Palette *palette = SDL_CreatePalette(256);
    ASSERT_NE(palette, nullptr);
    EXPECT_TRUE(s.setPalette(palette));
    EXPECT_EQ(s.getPalette(), palette);
    SDL_DestroyPalette(palette); // the surface keeps its own reference
    EXPECT_EQ(s.getPalette(), palette);
}

TEST(SurfaceTest, AlternateImages)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_FALSE(s.hasAlternateImages());
    std::vector<SDL_Surface *> images = s.getImages();
    ASSERT_EQ(images.size(), 1u);
    EXPECT_EQ(images[0], s.get());

    Surface big(SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA32));
    ASSERT_TRUE(big);
    EXPECT_TRUE(s.addAlternateImage(big.get()));
    EXPECT_TRUE(s.hasAlternateImages());
    images = s.getImages();
    ASSERT_EQ(images.size(), 2u);
    EXPECT_EQ(images[0], s.get());
    EXPECT_EQ(images[1], big.get());

    s.removeAlternateImages();
    EXPECT_FALSE(s.hasAlternateImages());
    EXPECT_EQ(s.getImages().size(), 1u);
}

TEST(SurfaceTest, Properties)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_NE(s.getProperties(), 0u);
}

TEST(SurfaceTest, AccessorsFailWithoutASurface)
{
    Surface s;
    Uint32 key = 0;
    Uint8 alpha = 0;
    EXPECT_FALSE(s.setColorKey(true, 0));
    EXPECT_FALSE(s.getColorKey(key));
    EXPECT_FALSE(s.setAlphaMod(0));
    EXPECT_FALSE(s.getAlphaMod(alpha));
    EXPECT_FALSE(s.hasRLE());
    EXPECT_EQ(s.getColorspace(), SDL_COLORSPACE_UNKNOWN);
    EXPECT_EQ(s.getPalette(), nullptr);
    EXPECT_TRUE(s.getImages().empty());
    EXPECT_EQ(s.getProperties(), 0u);
}

static_assert(!std::is_copy_constructible_v<Surface::Lock>, "Lock must not be copyable");
static_assert(!std::is_move_constructible_v<Surface::Lock>, "Lock must not be movable");
static_assert(!std::is_convertible_v<Surface &, Surface::Lock>, "Locking must be explicit");

namespace
{

// Returns a surface that must be locked to access its pixels. An RLE surface is encoded the first time it is
// blitted, and an encoded surface must be locked.
SDL_Surface *newSurfaceThatMustBeLocked()
{
    SDL_Surface *s = newSurface();
    SDL_Surface *dst = newSurface();
    if (s != nullptr && dst != nullptr)
    {
        SDL_SetSurfaceRLE(s, true);
        SDL_BlitSurface(s, nullptr, dst, nullptr);
    }
    SDL_DestroySurface(dst);
    return s;
}

bool isLocked(Surface const &s)
{
    return (s->flags & SDL_SURFACE_LOCKED) != 0;
}

} // namespace

TEST(SurfaceTest, MapRGB)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_EQ(s.mapRGB(1, 2, 3), SDL_MapSurfaceRGB(s.get(), 1, 2, 3));
    EXPECT_EQ(s.mapRGBA(1, 2, 3, 4), SDL_MapSurfaceRGBA(s.get(), 1, 2, 3, 4));
}

TEST(SurfaceTest, WriteThenReadPixel)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.writePixel(1, 2, 10, 20, 30, 40));
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = 0;
    EXPECT_TRUE(s.readPixel(1, 2, r, g, b, a));
    EXPECT_EQ(r, 10);
    EXPECT_EQ(g, 20);
    EXPECT_EQ(b, 30);
    EXPECT_EQ(a, 40);
}

TEST(SurfaceTest, WriteThenReadPixelFloat)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.writePixelFloat(3, 0, 1.0f, 0.0f, 1.0f, 1.0f));
    float r = 0.5f;
    float g = 0.5f;
    float b = 0.5f;
    float a = 0.5f;
    EXPECT_TRUE(s.readPixelFloat(3, 0, r, g, b, a));
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 1.0f);
    EXPECT_FLOAT_EQ(a, 1.0f);
}

TEST(SurfaceTest, PixelAccessOutsideTheSurfaceFails)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    Uint8 r, g, b, a;
    EXPECT_FALSE(s.writePixel(4, 0, 0, 0, 0, 0));
    EXPECT_FALSE(s.readPixel(0, -1, r, g, b, a));
}

TEST(SurfaceTest, PixelAccessFailsWithoutASurface)
{
    Surface s;
    Uint8 r, g, b, a;
    EXPECT_FALSE(s.writePixel(0, 0, 0, 0, 0, 0));
    EXPECT_FALSE(s.readPixel(0, 0, r, g, b, a));
}

TEST(SurfaceTest, LockDoesNotLockASurfaceThatDoesNotNeedIt)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    ASSERT_FALSE(SDL_MUSTLOCK(s.get()));
    {
        Surface::Lock lock(s);
        EXPECT_TRUE(lock);
        EXPECT_FALSE(isLocked(s));
    }
    EXPECT_FALSE(isLocked(s));
}

TEST(SurfaceTest, LockLocksAndUnlocksASurfaceThatMustBeLocked)
{
    Surface s(newSurfaceThatMustBeLocked());
    ASSERT_TRUE(s);
    ASSERT_TRUE(SDL_MUSTLOCK(s.get()));
    {
        Surface::Lock lock(s);
        EXPECT_TRUE(lock);
        EXPECT_TRUE(isLocked(s));
        EXPECT_NE(s->pixels, nullptr); // an RLE surface is decoded while locked
    }
    EXPECT_FALSE(isLocked(s));
}

TEST(SurfaceTest, NestedLocksUnlockWithTheLastOne)
{
    Surface s(newSurfaceThatMustBeLocked());
    ASSERT_TRUE(s);
    ASSERT_TRUE(SDL_MUSTLOCK(s.get()));
    {
        Surface::Lock outer(s);
        {
            Surface::Lock inner(s);
            EXPECT_TRUE(isLocked(s));
        }
        EXPECT_TRUE(isLocked(s));
    }
    EXPECT_FALSE(isLocked(s));
}

TEST(SurfaceTest, LockWithoutASurfaceIsNotValid)
{
    Surface s;
    Surface::Lock lock(s);
    EXPECT_FALSE(lock);
}

TEST(SurfaceTest, LockFunctionReturnsALock)
{
    Surface s(newSurfaceThatMustBeLocked());
    ASSERT_TRUE(s);
    ASSERT_TRUE(SDL_MUSTLOCK(s.get()));
    {
        auto lock = s.lock();
        EXPECT_TRUE(lock);
        EXPECT_TRUE(isLocked(s));
        EXPECT_NE(s->pixels, nullptr);
    }
    EXPECT_FALSE(isLocked(s));
}

namespace
{

struct Rgba
{
    Uint8 r = 0, g = 0, b = 0, a = 0;
};

Rgba pixelAt(Surface const &s, int x, int y)
{
    Rgba p;
    s.readPixel(x, y, p.r, p.g, p.b, p.a);
    return p;
}

bool isColor(Rgba p, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return p.r == r && p.g == g && p.b == b && p.a == a;
}

} // namespace

TEST(SurfaceTest, FillRectFillsOnlyTheRectangle)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    SDL_Rect const rect = {1, 1, 2, 2};
    EXPECT_TRUE(s.fillRect(&rect, s.mapRGBA(255, 0, 0, 255)));
    EXPECT_TRUE(isColor(pixelAt(s, 1, 1), 255, 0, 0, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 2, 2), 255, 0, 0, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 0, 0), 0, 0, 0, 0));
    EXPECT_TRUE(isColor(pixelAt(s, 3, 3), 0, 0, 0, 0));
}

TEST(SurfaceTest, FillRectWithNoRectangleFillsTheWholeSurface)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    EXPECT_TRUE(s.fillRect(nullptr, s.mapRGBA(0, 255, 0, 255)));
    EXPECT_TRUE(isColor(pixelAt(s, 0, 0), 0, 255, 0, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 3, 3), 0, 255, 0, 255));
}

TEST(SurfaceTest, FillRectsFillsEachRectangle)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    SDL_Rect const rects[] = {{0, 0, 1, 1}, {3, 3, 1, 1}};
    EXPECT_TRUE(s.fillRects(rects, 2, s.mapRGBA(0, 0, 255, 255)));
    EXPECT_TRUE(isColor(pixelAt(s, 0, 0), 0, 0, 255, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 3, 3), 0, 0, 255, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 1, 1), 0, 0, 0, 0));
}

TEST(SurfaceTest, ClearFillsTheWholeSurfaceIgnoringTheClipRect)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    SDL_Rect const clip = {0, 0, 1, 1};
    s.setClipRect(&clip);
    EXPECT_TRUE(s.clear(1.0f, 1.0f, 0.0f, 1.0f));
    EXPECT_TRUE(isColor(pixelAt(s, 0, 0), 255, 255, 0, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 3, 3), 255, 255, 0, 255));
}

TEST(SurfaceTest, FlipMovesPixels)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    s.writePixel(0, 0, 255, 0, 0, 255);
    EXPECT_TRUE(s.flip(SDL_FLIP_HORIZONTAL));
    EXPECT_TRUE(isColor(pixelAt(s, 3, 0), 255, 0, 0, 255));
    EXPECT_TRUE(isColor(pixelAt(s, 0, 0), 0, 0, 0, 0));
    EXPECT_TRUE(s.flip(SDL_FLIP_VERTICAL));
    EXPECT_TRUE(isColor(pixelAt(s, 3, 3), 255, 0, 0, 255));
}

TEST(SurfaceTest, PremultiplyAlphaScalesTheColor)
{
    Surface s(newSurface());
    ASSERT_TRUE(s);
    s.writePixel(0, 0, 200, 100, 50, 128);
    EXPECT_TRUE(s.premultiplyAlpha(false));
    Rgba p = pixelAt(s, 0, 0);
    EXPECT_NEAR(p.r, 200 * 128 / 255, 1);
    EXPECT_NEAR(p.g, 100 * 128 / 255, 1);
    EXPECT_NEAR(p.b, 50 * 128 / 255, 1);
    EXPECT_EQ(p.a, 128);
}

TEST(SurfaceTest, ChangesFailWithoutASurface)
{
    Surface s;
    EXPECT_FALSE(s.fillRect(nullptr, 0));
    EXPECT_FALSE(s.clear(0.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_FALSE(s.flip(SDL_FLIP_HORIZONTAL));
    EXPECT_FALSE(s.premultiplyAlpha(false));
}

namespace
{

// Returns a transparent black RGBA32 surface that copies its pixels when blitted, instead of blending them
Surface newOpaqueCopySurface(int w, int h)
{
    Surface s(SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32));
    if (s)
    {
        s.setBlendMode(SDL_BLENDMODE_NONE);
    }
    return s;
}

void paint(Surface &s, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    s.writePixel(x, y, r, g, b, 255);
}

bool isRed(Surface const &s, int x, int y)
{
    return isColor(pixelAt(s, x, y), 255, 0, 0, 255);
}

bool isGreen(Surface const &s, int x, int y)
{
    return isColor(pixelAt(s, x, y), 0, 255, 0, 255);
}

bool isUntouched(Surface const &s, int x, int y)
{
    return isColor(pixelAt(s, x, y), 0, 0, 0, 0);
}

} // namespace

TEST(SurfaceTest, BlitCopiesToTheLocation)
{
    Surface src = newOpaqueCopySurface(2, 2);
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    src.clear(1.0f, 0.0f, 0.0f, 1.0f);
    SDL_Rect const at = {2, 1, 0, 0}; // the width and height are ignored
    EXPECT_TRUE(dst.blit(src, nullptr, &at));
    EXPECT_TRUE(isRed(dst, 2, 1));
    EXPECT_TRUE(isRed(dst, 3, 2));
    EXPECT_TRUE(isUntouched(dst, 1, 1));
    EXPECT_TRUE(isUntouched(dst, 2, 3));
}

TEST(SurfaceTest, BlitCopiesOnlyTheSourceRectangle)
{
    Surface src = newOpaqueCopySurface(2, 1);
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    paint(src, 0, 0, 255, 0, 0);
    paint(src, 1, 0, 0, 255, 0);
    SDL_Rect const second = {1, 0, 1, 1};
    EXPECT_TRUE(dst.blit(src, &second, nullptr));
    EXPECT_TRUE(isGreen(dst, 0, 0));
    EXPECT_TRUE(isUntouched(dst, 1, 0));
}

TEST(SurfaceTest, BlitIsClippedToTheDestinationClipRect)
{
    Surface src = newOpaqueCopySurface(4, 4);
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    src.clear(1.0f, 0.0f, 0.0f, 1.0f);
    SDL_Rect const clip = {0, 0, 2, 2};
    dst.setClipRect(&clip);
    EXPECT_TRUE(dst.blit(src, nullptr, nullptr));
    EXPECT_TRUE(isRed(dst, 1, 1));
    EXPECT_TRUE(isUntouched(dst, 2, 2));
}

TEST(SurfaceTest, BlitUncheckedCopiesTheRectangles)
{
    Surface src = newOpaqueCopySurface(2, 2);
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    src.clear(1.0f, 0.0f, 0.0f, 1.0f);
    SDL_Rect const srcRect = {0, 0, 2, 2};
    SDL_Rect const dstRect = {1, 1, 2, 2};
    EXPECT_TRUE(dst.blitUnchecked(src, &srcRect, &dstRect));
    EXPECT_TRUE(isRed(dst, 1, 1));
    EXPECT_TRUE(isRed(dst, 2, 2));
    EXPECT_TRUE(isUntouched(dst, 0, 0));
    EXPECT_TRUE(isUntouched(dst, 3, 3));
}

TEST(SurfaceTest, BlitScaledFillsTheDestinationRectangle)
{
    Surface src = newOpaqueCopySurface(1, 1);
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    paint(src, 0, 0, 255, 0, 0);
    SDL_Rect const dstRect = {0, 0, 3, 2};
    EXPECT_TRUE(dst.blitScaled(src, nullptr, &dstRect, SDL_SCALEMODE_NEAREST));
    EXPECT_TRUE(isRed(dst, 0, 0));
    EXPECT_TRUE(isRed(dst, 2, 1));
    EXPECT_TRUE(isUntouched(dst, 3, 0));
    EXPECT_TRUE(isUntouched(dst, 0, 2));
}

TEST(SurfaceTest, BlitUncheckedScaledFillsTheDestinationRectangle)
{
    Surface src = newOpaqueCopySurface(1, 1);
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    paint(src, 0, 0, 255, 0, 0);
    SDL_Rect const srcRect = {0, 0, 1, 1};
    SDL_Rect const dstRect = {2, 2, 2, 2};
    EXPECT_TRUE(dst.blitUncheckedScaled(src, &srcRect, &dstRect, SDL_SCALEMODE_NEAREST));
    EXPECT_TRUE(isRed(dst, 2, 2));
    EXPECT_TRUE(isRed(dst, 3, 3));
    EXPECT_TRUE(isUntouched(dst, 1, 1));
}

TEST(SurfaceTest, StretchCopiesPixelsThatBlitWouldSkip)
{
    Surface src = newOpaqueCopySurface(1, 1);
    Surface dst = newOpaqueCopySurface(2, 2);
    ASSERT_TRUE(src && dst);
    paint(src, 0, 0, 255, 0, 0);
    src.setColorKey(true, src.mapRGB(255, 0, 0)); // the only pixel is transparent to a blit

    EXPECT_TRUE(dst.blitScaled(src, nullptr, nullptr, SDL_SCALEMODE_NEAREST));
    EXPECT_TRUE(isUntouched(dst, 0, 0));

    EXPECT_TRUE(dst.stretch(src, nullptr, nullptr, SDL_SCALEMODE_NEAREST));
    EXPECT_TRUE(isRed(dst, 0, 0));
    EXPECT_TRUE(isRed(dst, 1, 1));
}

TEST(SurfaceTest, BlitTiledRepeatsTheSource)
{
    Surface src = newOpaqueCopySurface(2, 1);
    Surface dst = newOpaqueCopySurface(5, 1);
    ASSERT_TRUE(src && dst);
    paint(src, 0, 0, 255, 0, 0);
    paint(src, 1, 0, 0, 255, 0);
    EXPECT_TRUE(dst.blitTiled(src, nullptr, nullptr));
    EXPECT_TRUE(isRed(dst, 0, 0));
    EXPECT_TRUE(isGreen(dst, 1, 0));
    EXPECT_TRUE(isRed(dst, 2, 0));
    EXPECT_TRUE(isGreen(dst, 3, 0));
    EXPECT_TRUE(isRed(dst, 4, 0));
}

TEST(SurfaceTest, BlitTiledWithScaleRepeatsTheScaledSource)
{
    Surface src = newOpaqueCopySurface(2, 1);
    Surface dst = newOpaqueCopySurface(8, 2);
    ASSERT_TRUE(src && dst);
    paint(src, 0, 0, 255, 0, 0);
    paint(src, 1, 0, 0, 255, 0);
    EXPECT_TRUE(dst.blitTiledWithScale(src, nullptr, 2.0f, SDL_SCALEMODE_NEAREST, nullptr));
    for (int x = 0; x < 8; ++x)
    {
        bool const red = (x / 2) % 2 == 0;
        EXPECT_EQ(isRed(dst, x, 0), red) << "x = " << x;
        EXPECT_EQ(isGreen(dst, x, 0), !red) << "x = " << x;
    }
}

TEST(SurfaceTest, Blit9GridKeepsTheCornersAndStretchesTheRest)
{
    // A 3x3 source with red corners and green edges and center
    Surface src = newOpaqueCopySurface(3, 3);
    Surface dst = newOpaqueCopySurface(6, 5);
    ASSERT_TRUE(src && dst);
    src.clear(0.0f, 1.0f, 0.0f, 1.0f);
    paint(src, 0, 0, 255, 0, 0);
    paint(src, 2, 0, 255, 0, 0);
    paint(src, 0, 2, 255, 0, 0);
    paint(src, 2, 2, 255, 0, 0);
    EXPECT_TRUE(dst.blit9Grid(src, nullptr, 1, 1, 1, 1, 0.0f, SDL_SCALEMODE_NEAREST, nullptr));
    EXPECT_TRUE(isRed(dst, 0, 0));
    EXPECT_TRUE(isRed(dst, 5, 0));
    EXPECT_TRUE(isRed(dst, 0, 4));
    EXPECT_TRUE(isRed(dst, 5, 4));
    EXPECT_TRUE(isGreen(dst, 1, 0)); // top edge
    EXPECT_TRUE(isGreen(dst, 4, 0));
    EXPECT_TRUE(isGreen(dst, 0, 1)); // left edge
    EXPECT_TRUE(isGreen(dst, 2, 2)); // center
}

TEST(SurfaceTest, BlitFailsWhileTheSourceIsLocked)
{
    Surface src(newSurfaceThatMustBeLocked());
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(src && dst);
    {
        auto lock = src.lock();
        EXPECT_FALSE(dst.blit(src, nullptr, nullptr));
    }
    EXPECT_TRUE(dst.blit(src, nullptr, nullptr));
}

TEST(SurfaceTest, BlitFailsWhileTheDestinationIsLocked)
{
    Surface src = newOpaqueCopySurface(4, 4);
    Surface dst(newSurfaceThatMustBeLocked());
    ASSERT_TRUE(src && dst);
    {
        auto lock = dst.lock();
        EXPECT_FALSE(dst.blit(src, nullptr, nullptr));
    }
    EXPECT_TRUE(dst.blit(src, nullptr, nullptr));
}

TEST(SurfaceTest, BlitsFailWithoutASource)
{
    Surface src;
    Surface dst = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(dst);
    EXPECT_FALSE(dst.blit(src, nullptr, nullptr));
    EXPECT_FALSE(dst.blitScaled(src, nullptr, nullptr, SDL_SCALEMODE_NEAREST));
    EXPECT_FALSE(dst.stretch(src, nullptr, nullptr, SDL_SCALEMODE_NEAREST));
    EXPECT_FALSE(dst.blitTiled(src, nullptr, nullptr));
    EXPECT_FALSE(dst.blitTiledWithScale(src, nullptr, 2.0f, SDL_SCALEMODE_NEAREST, nullptr));
    EXPECT_FALSE(dst.blit9Grid(src, nullptr, 1, 1, 1, 1, 0.0f, SDL_SCALEMODE_NEAREST, nullptr));
}

TEST(SurfaceTest, BlitsFailWithoutADestination)
{
    Surface src = newOpaqueCopySurface(4, 4);
    Surface dst;
    ASSERT_TRUE(src);
    EXPECT_FALSE(dst.blit(src, nullptr, nullptr));
    EXPECT_FALSE(dst.blitScaled(src, nullptr, nullptr, SDL_SCALEMODE_NEAREST));
    EXPECT_FALSE(dst.stretch(src, nullptr, nullptr, SDL_SCALEMODE_NEAREST));
    EXPECT_FALSE(dst.blitTiled(src, nullptr, nullptr));
    EXPECT_FALSE(dst.blitTiledWithScale(src, nullptr, 2.0f, SDL_SCALEMODE_NEAREST, nullptr));
    EXPECT_FALSE(dst.blit9Grid(src, nullptr, 1, 1, 1, 1, 0.0f, SDL_SCALEMODE_NEAREST, nullptr));
}

TEST(SurfaceTest, BlitAcceptsAConstSource)
{
    Surface source = newOpaqueCopySurface(1, 1);
    Surface dst = newOpaqueCopySurface(1, 1);
    ASSERT_TRUE(source && dst);
    paint(source, 0, 0, 255, 0, 0);
    Surface const &src = source;
    EXPECT_TRUE(dst.blit(src, nullptr, nullptr));
    EXPECT_TRUE(isRed(dst, 0, 0));
}

TEST(SurfaceTest, CloneIsAnIndependentCopy)
{
    Surface original = newOpaqueCopySurface(2, 2);
    ASSERT_TRUE(original);
    paint(original, 0, 0, 255, 0, 0);
    Surface copy = original.clone();
    ASSERT_TRUE(copy);
    EXPECT_NE(copy.get(), original.get());
    EXPECT_EQ(copy->w, 2);
    EXPECT_EQ(copy->h, 2);
    EXPECT_EQ(copy->format, original->format);
    EXPECT_TRUE(isRed(copy, 0, 0));

    paint(copy, 1, 1, 0, 255, 0);
    EXPECT_TRUE(isUntouched(original, 1, 1));
}

TEST(SurfaceTest, ConvertedChangesTheFormatAndKeepsThePixels)
{
    Surface original = newOpaqueCopySurface(2, 2);
    ASSERT_TRUE(original);
    paint(original, 1, 0, 255, 0, 0);
    Surface converted = original.converted(SDL_PIXELFORMAT_XRGB8888);
    ASSERT_TRUE(converted);
    EXPECT_EQ(converted->format, SDL_PIXELFORMAT_XRGB8888);
    EXPECT_EQ(original->format, SDL_PIXELFORMAT_RGBA32);
    EXPECT_TRUE(isRed(converted, 1, 0));
}

TEST(SurfaceTest, ConvertedWithAPaletteMapsToIt)
{
    Surface original = newOpaqueCopySurface(1, 1);
    ASSERT_TRUE(original);
    paint(original, 0, 0, 0, 255, 0);

    SDL_Palette *palette = SDL_CreatePalette(2);
    ASSERT_NE(palette, nullptr);
    SDL_Color const colors[] = {{255, 0, 0, 255}, {0, 255, 0, 255}};
    SDL_SetPaletteColors(palette, colors, 0, 2);

    Surface converted = original.converted(SDL_PIXELFORMAT_INDEX8, palette, SDL_COLORSPACE_SRGB, 0);
    SDL_DestroyPalette(palette); // the converted surface keeps its own reference
    ASSERT_TRUE(converted);
    EXPECT_EQ(converted->format, SDL_PIXELFORMAT_INDEX8);
    EXPECT_EQ(converted.getColorspace(), SDL_COLORSPACE_SRGB);
    EXPECT_EQ(static_cast<Uint8 const *>(converted->pixels)[0], 1); // the green entry
    EXPECT_TRUE(isGreen(converted, 0, 0));
}

TEST(SurfaceTest, ScaledMakesACopyOfTheNewSize)
{
    Surface original = newOpaqueCopySurface(1, 1);
    ASSERT_TRUE(original);
    paint(original, 0, 0, 255, 0, 0);
    Surface scaled = original.scaled(3, 2, SDL_SCALEMODE_NEAREST);
    ASSERT_TRUE(scaled);
    EXPECT_EQ(scaled->w, 3);
    EXPECT_EQ(scaled->h, 2);
    EXPECT_TRUE(isRed(scaled, 0, 0));
    EXPECT_TRUE(isRed(scaled, 2, 1));
    EXPECT_EQ(original->w, 1);
}

TEST(SurfaceTest, RotatedTurnsClockwise)
{
    Surface original = newOpaqueCopySurface(3, 2);
    ASSERT_TRUE(original);
    paint(original, 0, 0, 255, 0, 0); // top left
    Surface rotated = original.rotated(90.0f);
    ASSERT_TRUE(rotated);
    EXPECT_EQ(rotated->w, 2);
    EXPECT_EQ(rotated->h, 3);
    EXPECT_TRUE(isRed(rotated, 1, 0)); // the top left corner turns to the top right
    EXPECT_EQ(original->w, 3);
}

TEST(SurfaceTest, RotatedByAnOddAngleMakesALargerSurface)
{
    Surface original = newOpaqueCopySurface(4, 4);
    ASSERT_TRUE(original);
    Surface rotated = original.rotated(45.0f);
    ASSERT_TRUE(rotated);
    EXPECT_GT(rotated->w, 4);
    EXPECT_GT(rotated->h, 4);
}

TEST(SurfaceTest, NewSurfacesAreEmptyWithoutASurface)
{
    Surface s;
    EXPECT_FALSE(s.clone());
    EXPECT_FALSE(s.converted(SDL_PIXELFORMAT_RGBA32));
    EXPECT_FALSE(s.converted(SDL_PIXELFORMAT_RGBA32, nullptr, SDL_COLORSPACE_SRGB, 0));
    EXPECT_FALSE(s.scaled(2, 2, SDL_SCALEMODE_NEAREST));
    EXPECT_FALSE(s.rotated(90.0f));
}

TEST(SurfaceTest, ConstructorFromPixelsUsesTheData)
{
    std::vector<Uint32> data(4 * 2, 0);
    {
        Surface s(4, 2, SDL_PIXELFORMAT_RGBA32, data.data(), 4 * sizeof(Uint32));
        ASSERT_TRUE(s);
        EXPECT_EQ(s->w, 4);
        EXPECT_EQ(s->h, 2);
        EXPECT_EQ(s->pixels, data.data());
        EXPECT_TRUE((s->flags & SDL_SURFACE_PREALLOCATED) != 0); // SDL won't free the data

        EXPECT_TRUE(s.writePixel(1, 1, 255, 0, 0, 255));
        EXPECT_EQ(data[1 * 4 + 1], s.mapRGBA(255, 0, 0, 255));

        data[0] = s.mapRGBA(0, 255, 0, 255);
        EXPECT_TRUE(isGreen(s, 0, 0));
    }
    EXPECT_EQ(data.size(), 8u); // still usable after the surface is destroyed
}

TEST(SurfaceTest, ConstructorWithoutPixelsMakesASurfaceToFillInLater)
{
    Surface s(4, 4, SDL_PIXELFORMAT_RGBA32, nullptr, 0);
    ASSERT_TRUE(s);
    EXPECT_EQ(s->pixels, nullptr);
}

TEST(SurfaceTest, ConstructorFromPixelsCanFail)
{
    Uint32 pixel = 0;
    Surface s(-1, 1, SDL_PIXELFORMAT_RGBA32, &pixel, 4);
    EXPECT_FALSE(s);
}

namespace
{

// A path in the temporary directory that is deleted, if the file exists, when the TempFile is destroyed
class TempFile
{
  public:
    explicit TempFile(char const *name) : path_((std::filesystem::temp_directory_path() / name).string())
    {
        std::filesystem::remove(path_);
    }
    ~TempFile()
    {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }
    TempFile(TempFile const &) = delete;
    TempFile &operator=(TempFile const &) = delete;

    char const *c_str() const { return path_.c_str(); }

  private:
    std::string path_;
};

// Returns an opaque 3x2 surface with a red pixel at (0, 0) and a green pixel at (2, 1)
Surface newImage()
{
    Surface s = newOpaqueCopySurface(3, 2);
    if (s)
    {
        s.clear(0.0f, 0.0f, 1.0f, 1.0f);
        paint(s, 0, 0, 255, 0, 0);
        paint(s, 2, 1, 0, 255, 0);
    }
    return s;
}

bool matchesImage(Surface const &s)
{
    return s && s->w == 3 && s->h == 2 && isRed(s, 0, 0) && isGreen(s, 2, 1) &&
           isColor(pixelAt(s, 1, 0), 0, 0, 255, 255);
}

} // namespace

TEST(SurfaceTest, SaveBMPThenLoad)
{
    TempFile file("SdlxSurfaceTest_SaveBMPThenLoad.bmp");
    Surface image = newImage();
    ASSERT_TRUE(image);
    ASSERT_TRUE(image.saveBMP(file.c_str())) << SDL_GetError();
    EXPECT_TRUE(matchesImage(Surface::loadBMP(file.c_str())));
    EXPECT_TRUE(matchesImage(Surface::load(file.c_str())));
    EXPECT_FALSE(Surface::loadPNG(file.c_str())); // not a PNG
}

TEST(SurfaceTest, SavePNGThenLoad)
{
    TempFile file("SdlxSurfaceTest_SavePNGThenLoad.png");
    Surface image = newImage();
    ASSERT_TRUE(image);
    ASSERT_TRUE(image.savePNG(file.c_str())) << SDL_GetError();
    EXPECT_TRUE(matchesImage(Surface::loadPNG(file.c_str())));
    EXPECT_TRUE(matchesImage(Surface::load(file.c_str())));
    EXPECT_FALSE(Surface::loadBMP(file.c_str())); // not a BMP
}

TEST(SurfaceTest, LoadingAMissingFileFails)
{
    TempFile file("SdlxSurfaceTest_LoadingAMissingFileFails.bmp"); // removed, so it doesn't exist
    EXPECT_FALSE(Surface::load(file.c_str()));
    EXPECT_FALSE(Surface::loadBMP(file.c_str()));
    EXPECT_FALSE(Surface::loadPNG(file.c_str()));
}

TEST(SurfaceTest, SavingFailsWithoutASurface)
{
    TempFile file("SdlxSurfaceTest_SavingFailsWithoutASurface.bmp");
    Surface s;
    EXPECT_FALSE(s.saveBMP(file.c_str()));
    EXPECT_FALSE(s.savePNG(file.c_str()));
    EXPECT_FALSE(std::filesystem::exists(file.c_str()));
}

TEST(SurfaceTest, CopySharesTheSurface)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface a(raw);
        {
            Surface b(a);
            EXPECT_EQ(b.get(), raw);
            EXPECT_EQ(a.get(), raw);
            EXPECT_EQ(raw->refcount, 3);

            // A change through one copy is seen through the other
            EXPECT_TRUE(b.writePixel(0, 0, 255, 0, 0, 255));
            EXPECT_TRUE(isRed(a, 0, 0));
        }
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, CopyOfAnEmptySurfaceIsEmpty)
{
    Surface a;
    Surface b(a);
    EXPECT_FALSE(b);
    Surface c(newSurface());
    c = a;
    EXPECT_FALSE(c);
}

TEST(SurfaceTest, CopyAssignmentReleasesTheOldSurfaceAndSharesTheNewOne)
{
    SDL_Surface *first = newTrackedSurface();
    SDL_Surface *second = newTrackedSurface();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    {
        Surface a(first);
        Surface b(second);
        a = b;
        EXPECT_EQ(first->refcount, 1);
        EXPECT_EQ(a.get(), second);
        EXPECT_EQ(second->refcount, 3);
    }
    EXPECT_EQ(second->refcount, 1);
    SDL_DestroySurface(first);
    SDL_DestroySurface(second);
}

TEST(SurfaceTest, SelfCopyAssignmentKeepsTheCount)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface s(raw);
        Surface const &alias = s;
        s = alias;
        EXPECT_EQ(s.get(), raw);
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, CopyAssignmentBetweenSharersKeepsTheCount)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface a(raw);
        Surface b(a);
        a = b;
        EXPECT_EQ(raw->refcount, 3);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, MoveAssignmentBetweenSharersReleasesTheMovedReference)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface a(raw);
        Surface b(a);
        EXPECT_EQ(raw->refcount, 3);
        a = std::move(b);
        EXPECT_EQ(a.get(), raw);
        EXPECT_FALSE(b); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        EXPECT_EQ(raw->refcount, 2);
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, CopiesInAContainerShareTheSurface)
{
    SDL_Surface *raw = newTrackedSurface();
    ASSERT_NE(raw, nullptr);
    {
        Surface s(raw);
        std::vector<Surface> copies(3, s);
        EXPECT_EQ(raw->refcount, 5);
        for (Surface const &copy : copies)
        {
            EXPECT_EQ(copy.get(), raw);
        }
    }
    EXPECT_EQ(raw->refcount, 1);
    SDL_DestroySurface(raw);
}

TEST(SurfaceTest, ConstructorCreatesAClearedSurface)
{
    Surface s(3, 2, SDL_PIXELFORMAT_RGBA32);
    ASSERT_TRUE(s);
    EXPECT_EQ(s->w, 3);
    EXPECT_EQ(s->h, 2);
    EXPECT_EQ(s->format, SDL_PIXELFORMAT_RGBA32);
    EXPECT_NE(s->pixels, nullptr);
    EXPECT_FALSE((s->flags & SDL_SURFACE_PREALLOCATED) != 0); // SDL owns and frees the data
    EXPECT_EQ(s->refcount, 1);
    EXPECT_TRUE(isUntouched(s, 0, 0));
    EXPECT_TRUE(isUntouched(s, 2, 1));
}

TEST(SurfaceTest, ConstructorCanFail)
{
    Surface s(-1, 2, SDL_PIXELFORMAT_RGBA32);
    EXPECT_FALSE(s);
}
