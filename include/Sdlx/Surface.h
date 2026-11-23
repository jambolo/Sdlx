#pragma once

#include <SDL3/SDL.h>

#include <vector>

namespace Sdlx
{

//! An owning wrapper for an SDL_Surface.
//
//! A Surface owns one reference to at most one SDL_Surface, and releases it with SDL_DestroySurface when the Surface
//! is destroyed or given a different surface. SDL frees the surface when its last reference is released. A
//! default-constructed or moved-from Surface owns nothing.
//!
//! Copying a Surface adds a reference (using the SDL_Surface's refcount), so the copies share one SDL_Surface: a
//! change made through one, such as to its pixels or color key, is seen through all of them. Use clone for an
//! independent copy. Moving a Surface transfers its reference without changing the count.
//!
//! The wrapper never converts implicitly to SDL_Surface *. Call get to pass the surface to an SDL function, or
//! release to give up ownership of it.
//!
//! @warning    SDL's reference count is not atomic. Don't copy or destroy Surfaces that share an SDL_Surface on
//!             different threads at the same time.

class Surface
{
  public:
    //! Creates a surface with its own pixel data, cleared to zero. On failure, the Surface owns nothing; call
    //! SDL_GetError for the reason.
    //!
    //! @param	width,height	The size of the surface
    //! @param	format			The pixel format
    Surface(int width, int height, SDL_PixelFormat format) : surface_(SDL_CreateSurface(width, height, format)) {}

    //! Creates a surface that uses existing pixel data. On failure, the Surface owns nothing; call SDL_GetError for
    //! the reason.
    //!
    //! @param	width,height	The size of the surface
    //! @param	format			The pixel format of the data
    //! @param	pixels			The pixel data, or nullptr to set it later
    //! @param	pitch			The distance in bytes between rows, including padding, or 0 if pixels is nullptr
    //! @warning    The pixel data is not copied, and the Surface doesn't free it. The data must outlive the surface,
    //!             including any references to the surface that SDL or other code holds.
    Surface(int width, int height, SDL_PixelFormat format, void *pixels, int pitch)
        : surface_(SDL_CreateSurfaceFrom(width, height, format, pixels, pitch))
    {
    }

    //! Takes ownership of a surface.
    //!
    //! @param  surface		The surface to own. It may be nullptr.
    //! @warning   Ownership is transferred. Do not call SDL_DestroySurface.
    explicit Surface(SDL_Surface *surface = nullptr) noexcept : surface_(surface) {}

    // Destructor.
    ~Surface() { SDL_DestroySurface(surface_); }

    //! Copy constructor.
    //!
    //! @param	other	The Surface to share
    //! @note   The copy shares the other Surface's SDL_Surface.
    Surface(Surface const &other) noexcept : surface_(other.surface_)
    {
        if (surface_ != nullptr)
        {
            ++surface_->refcount;
        }
    }

    //! Copy-assignment operator.
    //!
    //! @param	other	The Surface to share
    //! @note   The copy shares the other Surface's SDL_Surface.
    Surface &operator=(Surface const &other) noexcept;

    //! Move constructor.
    Surface(Surface &&other) noexcept : surface_(other.release()) {}

    //! Move-assignment operator.
    Surface &operator=(Surface &&other) noexcept;

    //! Returns the owned surface, or nullptr if there is none.
    //! 
    //! @note   The returned pointer is borrowed. Do not call SDL_DestroySurface on it.
    SDL_Surface *get() const noexcept { return surface_; }

    //! Releases ownership of the owned SDL_Surface and returns it, or nullptr if there is none.
    //! 
    //! @note   Ownership is transferred. The caller must call SDL_DestroySurface on the returned pointer.
    SDL_Surface *release() noexcept;

    //! Gains ownership of a new SDL_Surface, releasing the old one if there is one.
    //!
    //! @param	surface		The SDL_Surface to own. It may be nullptr.
    //! @warning   Ownership is transferred. Do not call SDL_DestroySurface.
    void reset(SDL_Surface *surface = nullptr) noexcept;

    //! Exchanges the SDL_Surfaces of two Surfaces.
    //!
    //! @param	other		The Surface to exchange with
    void swap(Surface &other) noexcept;

    //! Exchanges the SDL_Surfaces of two Surfaces. Found by argument-dependent lookup, so call it unqualified:
    //! `using std::swap; swap(a, b);`
    //!
    //! @param	a,b		The Surfaces to exchange
    friend void swap(Surface &a, Surface &b) noexcept { a.swap(b); }

    //! Returns true if a surface is owned.
    explicit operator bool() const noexcept { return surface_ != nullptr; }

    //! Accesses the members of the owned SDL_Surface.
    //!
    //! @warning    The behavior is undefined if no SDL_Surface is owned.
    SDL_Surface *operator->() const noexcept;

    //! @name Surface values
    //! These wrap the SDL_Surface get and set functions. A function that returns bool returns false on failure
    //! (for example, when no surface is owned); call SDL_GetError for the reason.
    //! @{

    //! Sets or clears the color key.
    //! 
    //! @param	enabled		true to set the color key, false to clear it
    //! @param	key			The color key, a pixel value in the surface's format (see SDL_MapSurfaceRGB)
    bool setColorKey(bool enabled, Uint32 key)
    {
        return SDL_SetSurfaceColorKey(surface_, enabled, key);
    }

    //! Returns true if the surface has a color key.
    bool hasColorKey() const
    {
        return SDL_SurfaceHasColorKey(surface_);
    }

    //! Gets the color key.
    //! 
    //! @param	key		Receives the color key
    //! @return false if the surface has no color key.
    bool getColorKey(Uint32 &key) const
    {
        return SDL_GetSurfaceColorKey(surface_, &key);
    }

    //! Sets the color modulation used in blits.
    //!
    //! @param	r,g,b	The values multiplied into each color component
    bool setColorMod(Uint8 r, Uint8 g, Uint8 b)
    {
        return SDL_SetSurfaceColorMod(surface_, r, g, b);
    }

    //! Gets the color modulation used in blits.
    //!
    //! @param	r,g,b	Receive the values multiplied into each color component
    bool getColorMod(Uint8 &r, Uint8 &g, Uint8 &b) const
    {
        return SDL_GetSurfaceColorMod(surface_, &r, &g, &b);
    }

    //! Sets the alpha modulation used in blits.
    //!
    //! @param	alpha	The value multiplied into the alpha component
    bool setAlphaMod(Uint8 alpha)
    {
        return SDL_SetSurfaceAlphaMod(surface_, alpha);
    }

    //! Gets the alpha modulation used in blits.
    //!
    //! @param	alpha	Receives the value multiplied into the alpha component
    bool getAlphaMod(Uint8 &alpha) const
    {
        return SDL_GetSurfaceAlphaMod(surface_, &alpha);
    }

    //! Sets the blend mode used in blits.
    //!
    //! @param	blendMode	The blend mode
    bool setBlendMode(SDL_BlendMode blendMode)
    {
        return SDL_SetSurfaceBlendMode(surface_, blendMode);
    }

    //! Gets the blend mode used in blits.
    //!
    //! @param	blendMode	Receives the blend mode
    bool getBlendMode(SDL_BlendMode &blendMode) const
    {
        return SDL_GetSurfaceBlendMode(surface_, &blendMode);
    }

    //! Sets the clip rectangle for blits to this surface.
    //! 
    //! @param	rect	The clip rectangle, or nullptr to clip to the whole surface
    //! @return false, if the rectangle doesn't intersect the surface.
    bool setClipRect(SDL_Rect const *rect)
    {
        return SDL_SetSurfaceClipRect(surface_, rect);
    }

    //! Gets the clip rectangle for blits to this surface.
    //!
    //! @param	rect	Receives the clip rectangle
    bool getClipRect(SDL_Rect &rect) const
    {
        return SDL_GetSurfaceClipRect(surface_, &rect);
    }

    //! Enables or disables RLE acceleration.
    //!
    //! @param	enabled		true to enable RLE acceleration
    //! @note	With SDL3, an RLE surface with an alpha channel ignores its color key when blitted.
    bool setRLE(bool enabled)
    {
        return SDL_SetSurfaceRLE(surface_, enabled);
    }

    //! Returns true if RLE acceleration is enabled.
    bool hasRLE() const
    {
        return SDL_SurfaceHasRLE(surface_);
    }

    //! Sets the colorspace.
    //!
    //! @param	colorspace	The colorspace
    bool setColorspace(SDL_Colorspace colorspace)
    {
        return SDL_SetSurfaceColorspace(surface_, colorspace);
    }

    //! Returns the colorspace, or SDL_COLORSPACE_UNKNOWN if no SDL_Surface is owned.
    SDL_Colorspace getColorspace() const
    {
        return SDL_GetSurfaceColorspace(surface_);
    }

    //! Creates a palette for an indexed surface and returns it, or nullptr on failure.
    //!
    //! @warning The returned palette is borrowed from the surface. Do not call SDL_DestroyPalette on it.
    SDL_Palette *createPalette()
    {
        return SDL_CreateSurfacePalette(surface_);
    }

    //! Sets the palette of an indexed surface.
    //!
    //! @param	palette		The palette, or nullptr to remove the current palette
    //! @note   The surface adds a reference to the new palette and releases the old palette's reference.
    bool setPalette(SDL_Palette *palette)
    {
        return SDL_SetSurfacePalette(surface_, palette);
    }

    //! Returns the palette, or nullptr if there is none.
    //!
    //!@warning The returned palette is borrowed from the surface. Do not call SDL_DestroyPalette on it.
    SDL_Palette *getPalette() const
    {
        return SDL_GetSurfacePalette(surface_);
    }

    //! Adds an alternate version of the image.
    //!
    //! @param	image	The alternate image
    //! @note The surface adds a reference the image.
    bool addAlternateImage(SDL_Surface *image)
    {
        return SDL_AddSurfaceAlternateImage(surface_, image);
    }

    //! Returns true if the surface has alternate images.
    bool hasAlternateImages() const
    {
        return SDL_SurfaceHasAlternateImages(surface_);
    }

    //! Removes all alternate images.
    //! 
    //! @note The surface releases its references to the alternate images.
    void removeAlternateImages()
    {
        SDL_RemoveSurfaceAlternateImages(surface_);
    }

    //! Returns the surface image followed by its alternate images, or an empty vector on failure.
    //! 
    //!@warning   The returned images are borrowed. Do not call SDL_DestroySurface.
    std::vector<SDL_Surface *> getImages() const;

    //! Returns the surface's properties, or 0 on failure.
    SDL_PropertiesID getProperties() const
    {
        return SDL_GetSurfaceProperties(surface_);
    }

    //! @}

    //! @name Pixel access
    //! These wrap the SDL_Surface pixel functions. The read and write functions lock the surface themselves if
    //! necessary, so they don't need a Lock. A function that returns bool returns false on failure (for example,
    //! when no surface is owned or the coordinates are outside the surface); call SDL_GetError for the reason.
    //! @{

    //! Returns the pixel value of an opaque color in the surface's format.
    //!
    //! @param	r,g,b	The color components
    //! @note   On a paletted surface, the value is the index of the nearest palette entry.
    Uint32 mapRGB(Uint8 r, Uint8 g, Uint8 b) const
    {
        return SDL_MapSurfaceRGB(surface_, r, g, b);
    }

    //! Returns the pixel value of a color in the surface's format.
    //!
    //! @param	r,g,b,a		The color components
    //! @note   On a paletted surface, the value is the index of the nearest palette entry.
    Uint32 mapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const
    {
        return SDL_MapSurfaceRGBA(surface_, r, g, b, a);
    }

    //! Reads the color of a pixel.
    //!
    //! @param	x,y			The location of the pixel
    //! @param	r,g,b,a		Receive the color components
    bool readPixel(int x, int y, Uint8 &r, Uint8 &g, Uint8 &b, Uint8 &a) const
    {
        return SDL_ReadSurfacePixel(surface_, x, y, &r, &g, &b, &a);
    }

    //! Reads the color of a pixel, as components from 0 to 1.
    //!
    //! @param	x,y			The location of the pixel
    //! @param	r,g,b,a		Receive the color components
    bool readPixelFloat(int x, int y, float &r, float &g, float &b, float &a) const
    {
        return SDL_ReadSurfacePixelFloat(surface_, x, y, &r, &g, &b, &a);
    }

    //! Writes the color of a pixel.
    //!
    //! @param	x,y			The location of the pixel
    //! @param	r,g,b,a		The color components
    bool writePixel(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return SDL_WriteSurfacePixel(surface_, x, y, r, g, b, a);
    }

    //! Writes the color of a pixel, as components from 0 to 1.
    //!
    //! @param	x,y			The location of the pixel
    //! @param	r,g,b,a		The color components
    bool writePixelFloat(int x, int y, float r, float g, float b, float a)
    {
        return SDL_WriteSurfacePixelFloat(surface_, x, y, r, g, b, a);
    }

    //! @}

    //! @name Changing the surface
    //! These wrap the SDL functions that change a surface in place. They return false on failure (for example,
    //! when no surface is owned); call SDL_GetError for the reason.
    //! @{

    //! Fills a rectangle with a color. The fill is clipped to the clip rectangle.
    //!
    //! @param	rect	The rectangle to fill, or nullptr to fill the whole surface
    //! @param	color	The color, a pixel value in the surface's format (see mapRGB)
    bool fillRect(SDL_Rect const *rect, Uint32 color)
    {
        return SDL_FillSurfaceRect(surface_, rect, color);
    }

    //! Fills rectangles with a color. The fills are clipped to the clip rectangle.
    //!
    //! @param	rects	The rectangles to fill
    //! @param	count	The number of rectangles
    //! @param	color	The color, a pixel value in the surface's format (see mapRGB)
    bool fillRects(SDL_Rect const *rects, int count, Uint32 color)
    {
        return SDL_FillSurfaceRects(surface_, rects, count, color);
    }

    //! Fills the whole surface with a color, ignoring the clip rectangle.
    //!
    //! @param	r,g,b,a		The color components, from 0 to 1. Values above 1 are allowed for HDR surfaces.
    bool clear(float r, float g, float b, float a)
    {
        return SDL_ClearSurface(surface_, r, g, b, a);
    }

    //! Flips the image.
    //!
    //! @param	flipMode	The direction to flip
    bool flip(SDL_FlipMode flipMode)
    {
        return SDL_FlipSurface(surface_, flipMode);
    }

    //! Multiplies the color components of each pixel by its alpha.
    //!
    //! @param	linear	true to convert from sRGB to linear space before multiplying, and back afterward
    bool premultiplyAlpha(bool linear)
    {
        return SDL_PremultiplySurfaceAlpha(surface_, linear);
    }

    //! @}

    //! @name Blitting
    //! These wrap the SDL blit functions, with this surface as the destination. They return false on failure (for
    //! example, when either Surface owns nothing, or either surface is locked); call SDL_GetError for the reason.
    //!
    //! The source's blend mode, color key, color and alpha modulation apply, except for stretch, which copies the
    //! pixels. This surface's clip rectangle applies to all of them except the unchecked blits.
    //! @{

    //! Copies a rectangle of a source surface to a location in this surface, converting the format if needed.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to copy, or nullptr for the whole source
    //! @param	dstRect		The location in this surface (the width and height are ignored), or nullptr for (0, 0)
    bool blit(Surface const &src, SDL_Rect const *srcRect, SDL_Rect const *dstRect)
    {
        return SDL_BlitSurface(src.get(), srcRect, surface_, dstRect);
    }

    //! Copies a rectangle of a source surface to this surface, skipping the clipping and checks that blit does.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to copy. It must not be nullptr.
    //! @param	dstRect		The rectangle in this surface. It must not be nullptr.
    //! @warning    The rectangles must already be clipped to both surfaces, and neither surface may be locked.
    bool blitUnchecked(Surface const &src, SDL_Rect const *srcRect, SDL_Rect const *dstRect)
    {
        return SDL_BlitSurfaceUnchecked(src.get(), srcRect, surface_, dstRect);
    }

    //! Copies a rectangle of a source surface to a rectangle in this surface, scaling it to fit and converting the
    //! format if needed.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to copy, or nullptr for the whole source
    //! @param	dstRect		The rectangle in this surface, or nullptr for the whole surface
    //! @param	scaleMode	How to scale
    bool blitScaled(Surface const &src, SDL_Rect const *srcRect, SDL_Rect const *dstRect, SDL_ScaleMode scaleMode)
    {
        return SDL_BlitSurfaceScaled(src.get(), srcRect, surface_, dstRect, scaleMode);
    }

    //! Copies and scales a rectangle of a source surface to this surface, skipping the clipping and checks that
    //! blitScaled does.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to copy. It must not be nullptr.
    //! @param	dstRect		The rectangle in this surface. It must not be nullptr.
    //! @param	scaleMode	How to scale
    //! @warning    The rectangles must already be clipped to both surfaces, and neither surface may be locked.
    bool blitUncheckedScaled(Surface const &src, SDL_Rect const *srcRect, SDL_Rect const *dstRect,
                             SDL_ScaleMode scaleMode)
    {
        return SDL_BlitSurfaceUncheckedScaled(src.get(), srcRect, surface_, dstRect, scaleMode);
    }

    //! Copies the pixels of a rectangle of a source surface to a rectangle in this surface, scaling them to fit.
    //! Unlike blitScaled, it doesn't blend or apply a color key or modulation.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to copy, or nullptr for the whole source
    //! @param	dstRect		The rectangle in this surface, or nullptr for the whole surface
    //! @param	scaleMode	How to scale
    bool stretch(Surface const &src, SDL_Rect const *srcRect, SDL_Rect const *dstRect, SDL_ScaleMode scaleMode)
    {
        return SDL_StretchSurface(src.get(), srcRect, surface_, dstRect, scaleMode);
    }

    //! Fills a rectangle in this surface by repeating a rectangle of a source surface.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to repeat, or nullptr for the whole source
    //! @param	dstRect		The rectangle to fill, or nullptr for the whole surface
    bool blitTiled(Surface const &src, SDL_Rect const *srcRect, SDL_Rect const *dstRect)
    {
        return SDL_BlitSurfaceTiled(src.get(), srcRect, surface_, dstRect);
    }

    //! Fills a rectangle in this surface by scaling and repeating a rectangle of a source surface.
    //!
    //! @param	src			The source surface
    //! @param	srcRect		The rectangle to repeat, or nullptr for the whole source
    //! @param	scale		The scale applied to each tile (2 doubles its size)
    //! @param	scaleMode	How to scale
    //! @param	dstRect		The rectangle to fill, or nullptr for the whole surface
    bool blitTiledWithScale(Surface const &src, SDL_Rect const *srcRect, float scale, SDL_ScaleMode scaleMode,
                            SDL_Rect const *dstRect)
    {
        return SDL_BlitSurfaceTiledWithScale(src.get(), srcRect, scale, scaleMode, surface_, dstRect);
    }

    //! Draws a rectangle of a source surface as a 9-grid into a rectangle in this surface. The corners keep their
    //! size (times scale), the edges are stretched along their length, and the center is stretched to fill the
    //! rest.
    //!
    //! @param	src				The source surface
    //! @param	srcRect			The rectangle to use as the 9-grid, or nullptr for the whole source
    //! @param	leftWidth		The width of the left corners in srcRect
    //! @param	rightWidth		The width of the right corners in srcRect
    //! @param	topHeight		The height of the top corners in srcRect
    //! @param	bottomHeight	The height of the bottom corners in srcRect
    //! @param	scale			The scale applied to the corners, or 0 for no scaling
    //! @param	scaleMode		How to scale
    //! @param	dstRect			The rectangle to fill, or nullptr for the whole surface
    bool blit9Grid(Surface const &src, SDL_Rect const *srcRect, int leftWidth, int rightWidth, int topHeight,
                   int bottomHeight, float scale, SDL_ScaleMode scaleMode, SDL_Rect const *dstRect)
    {
        return SDL_BlitSurface9Grid(src.get(), srcRect, leftWidth, rightWidth, topHeight, bottomHeight, scale,
                                    scaleMode, surface_, dstRect);
    }

    //! @}

    //! @name New surfaces
    //! These wrap the SDL functions that make a new surface from this one. This surface is unchanged. Each returns
    //! a new Surface that owns the result, or a Surface that owns nothing on failure (for example, when this
    //! Surface owns nothing); call SDL_GetError for the reason.
    //! @{

    //! Returns a copy of this surface.
    //!
    //! @note   The copy shares this surface's alternate images.
    [[nodiscard]] Surface clone() const { return Surface(SDL_DuplicateSurface(surface_)); }

    //! Returns a copy of this surface in another pixel format.
    //!
    //! @param	format	The pixel format of the copy
    //! @note   A surface converted to the format of the surface it will be blitted to blits faster. The copy shares
    //!         this surface's alternate images.
    [[nodiscard]] Surface converted(SDL_PixelFormat format) const
    {
        return Surface(SDL_ConvertSurface(surface_, format));
    }

    //! Returns a copy of this surface in another pixel format and colorspace.
    //!
    //! @param	format		The pixel format of the copy
    //! @param	palette		The palette to map colors to for an indexed format, or nullptr
    //! @param	colorspace	The colorspace of the copy
    //! @param	props		Additional color properties, or 0
    //! @note   The copy shares this surface's alternate images.
    [[nodiscard]] Surface converted(SDL_PixelFormat format, SDL_Palette *palette, SDL_Colorspace colorspace,
                                    SDL_PropertiesID props) const
    {
        return Surface(SDL_ConvertSurfaceAndColorspace(surface_, format, palette, colorspace, props));
    }

    //! Returns a copy of this surface scaled to a new size.
    //!
    //! @param	width,height	The size of the copy
    //! @param	scaleMode		How to scale
    [[nodiscard]] Surface scaled(int width, int height, SDL_ScaleMode scaleMode) const
    {
        return Surface(SDL_ScaleSurface(surface_, width, height, scaleMode));
    }

    //! Returns a copy of this surface rotated clockwise.
    //!
    //! @param	angle	The angle in degrees. A negative angle rotates counterclockwise.
    //! @note   Unless the angle is a multiple of 90 degrees, the copy is larger, to hold the rotated image. The
    //!         corners are filled with the color key, if there is one, or transparent white.
    [[nodiscard]] Surface rotated(float angle) const { return Surface(SDL_RotateSurface(surface_, angle)); }

    //! @}

    //! @name Loading and saving
    //! These wrap SDL's own BMP and PNG functions; they don't use SDL3_image. A load function returns a Surface that
    //! owns nothing on failure, and a save function returns false; call SDL_GetError for the reason.
    //! @{

    //! Loads a BMP or PNG file.
    //!
    //! @param	file	The file to load
    [[nodiscard]] static Surface load(char const *file) { return Surface(SDL_LoadSurface(file)); }

    //! Loads a BMP file.
    //!
    //! @param	file	The file to load
    [[nodiscard]] static Surface loadBMP(char const *file) { return Surface(SDL_LoadBMP(file)); }

    //! Loads a PNG file.
    //!
    //! @param	file	The file to load
    //! @warning    Load PNG files only from trusted sources. SDL's PNG loader isn't hardened against malicious files.
    [[nodiscard]] static Surface loadPNG(char const *file) { return Surface(SDL_LoadPNG(file)); }

    //! Saves the surface to a BMP file.
    //!
    //! @param	file	The file to save to
    //! @note   24-bit, 32-bit and 8-bit paletted surfaces are saved as they are. Other formats are converted to 24-bit,
    //!         or to 32-bit if they have alpha or a color key. YUV and 1-bit and 4-bit paletted formats can't be saved.
    bool saveBMP(char const *file) const { return SDL_SaveBMP(surface_, file); }

    //! Saves the surface to a PNG file.
    //!
    //! @param	file	The file to save to
    bool savePNG(char const *file) const { return SDL_SavePNG(surface_, file); }

    //! @}

    //! A lock on a Surface's pixels, held for the lifetime of the Lock.
    //
    //! A Lock makes the pixels of a surface safe to access directly. The constructor locks the surface only if
    //! SDL_MUSTLOCK says it must be locked, and the destructor unlocks it only if the constructor locked it. Locks
    //! nest, so a surface stays locked until its last Lock is destroyed. A Lock can't be copied or moved.
    //!
    //! @warning    The Surface must keep its SDL_Surface for as long as the Lock exists. Don't blit to or from a
    //!             locked surface; SDL fails the blit.

    class Lock
    {
      public:
        //! Locks the surface, if it must be locked.
        //!
        //! @param	surface		The Surface to lock. It may own nothing, in which case the Lock is not valid.
        explicit Lock(Surface &surface);

        //! Unlocks the surface, if the constructor locked it.
        ~Lock();

        Lock(Lock const &) = delete;
        Lock &operator=(Lock const &) = delete;

        //! Returns true if the lock succeeded.
        explicit operator bool() const noexcept { return surface_ != nullptr; }

      private:
        SDL_Surface *surface_; // The locked surface, or nullptr if the Lock is not valid
        bool locked_;          // True if the constructor called SDL_LockSurface
    };

    //! Locks the surface, if it must be locked, for the lifetime of the returned Lock.
    //!
    //! @return A Lock on this Surface: `auto lock = surface.lock();`
    //! @warning    Keep the result. A discarded Lock unlocks the surface at the end of the statement.
    [[nodiscard]] Lock lock() { return Lock(*this); }

  private:
    SDL_Surface *surface_ = nullptr; // The owned surface
};

} // namespace Sdlx
