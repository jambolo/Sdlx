#pragma once

#include "Surface.h"

#include <SDL3/SDL.h>

#include <cstddef>
#include <vector>

namespace Sdlx
{

//! An owning wrapper for an SDL_Window.
//
//! A Window owns at most one SDL_Window, and destroys it with SDL_DestroyWindow when the Window is destroyed or given
//! a different window. A default-constructed or moved-from Window owns nothing.
//!
//! Unlike an SDL_Surface, an SDL_Window has no reference count, so a Window can be moved but not copied.
//!
//! The wrapper never converts implicitly to SDL_Window *. Call get to pass the window to an SDL function, or release
//! to give up ownership of it.
//!
//! @warning    SDL destroys a window's child windows (popups, and windows given a parent with setParent) when it
//!             destroys the window. Destroy or release a Window that owns a child before the Window that owns its
//!             parent.
//! @warning    Create, use and destroy Windows only on the main thread, as SDL requires.

class Window
{
  public:
    //! Creates a window. On failure, the Window owns nothing; call SDL_GetError for the reason.
    //!
    //! @param	title			The title of the window, in UTF-8
    //! @param	width,height	The size of the window, in screen coordinates
    //! @param	flags			Zero or more SDL_WINDOW_* flags, combined with |
    //! @note   The SDL video subsystem must be initialized.
    Window(char const *title, int width, int height, SDL_WindowFlags flags = 0)
        : window_(SDL_CreateWindow(title, width, height, flags))
    {
    }

    //! Creates a window with properties (see SDL_CreateWindowWithProperties). On failure, the Window owns nothing;
    //! call SDL_GetError for the reason.
    //!
    //! @param	props	The properties of the window
    //! @note   The SDL video subsystem must be initialized.
    explicit Window(SDL_PropertiesID props) : window_(SDL_CreateWindowWithProperties(props)) {}

    //! Takes ownership of a window.
    //!
    //! @param	window	The window to own. It may be nullptr.
    //! @warning   Ownership is transferred. Do not call SDL_DestroyWindow.
    explicit Window(SDL_Window *window = nullptr) noexcept : window_(window) {}

    // Destructor.
    ~Window() { destroy(window_); }

    Window(Window const &) = delete;
    Window &operator=(Window const &) = delete;

    //! Move constructor.
    Window(Window &&other) noexcept : window_(other.release()) {}

    //! Move-assignment operator.
    Window &operator=(Window &&other) noexcept;

    //! Returns the owned window, or nullptr if there is none.
    //!
    //! @note   The returned pointer is borrowed. Do not call SDL_DestroyWindow on it.
    SDL_Window *get() const noexcept { return window_; }

    //! Releases ownership of the owned SDL_Window and returns it, or nullptr if there is none.
    //!
    //! @note   Ownership is transferred. The caller must call SDL_DestroyWindow on the returned pointer.
    SDL_Window *release() noexcept;

    //! Gains ownership of a new SDL_Window, destroying the old one if there is one.
    //!
    //! @param	window	The SDL_Window to own. It may be nullptr.
    //! @warning   Ownership is transferred. Do not call SDL_DestroyWindow.
    void reset(SDL_Window *window = nullptr) noexcept;

    //! Exchanges the SDL_Windows of two Windows.
    //!
    //! @param	other	The Window to exchange with
    void swap(Window &other) noexcept;

    //! Exchanges the SDL_Windows of two Windows. Found by argument-dependent lookup, so call it unqualified:
    //! `using std::swap; swap(a, b);`
    //!
    //! @param	a,b		The Windows to exchange
    friend void swap(Window &a, Window &b) noexcept { a.swap(b); }

    //! Returns true if a window is owned.
    explicit operator bool() const noexcept { return window_ != nullptr; }

    //! Creates a popup window (a tooltip or a menu) with this window as its parent. On failure, the returned Window
    //! owns nothing; call SDL_GetError for the reason.
    //!
    //! @param	offsetX,offsetY		The position of the popup relative to this window's origin
    //! @param	width,height		The size of the popup
    //! @param	flags				SDL_WINDOW_TOOLTIP or SDL_WINDOW_POPUP_MENU, and zero or more other SDL_WINDOW_*
    //!								flags
    //! @warning    SDL destroys the popup when it destroys this window. Destroy or release the returned Window first.
    [[nodiscard]] Window createPopup(int offsetX, int offsetY, int width, int height, SDL_WindowFlags flags) const;

    //! @name Window values
    //! These wrap the SDL_Window get and set functions. A function that returns bool returns false on failure (for
    //! example, when no window is owned); call SDL_GetError for the reason. A size or position is in screen
    //! coordinates, unless the name says pixels.
    //!
    //! Some changes, such as to the size or position, are requests that the window system may carry out later, or
    //! not at all. Call sync to wait for them.
    //! @{

    //! Returns the window's ID, or 0 on failure. SDL window events identify their window by this ID.
    SDL_WindowID getID() const { return SDL_GetWindowID(window_); }

    //! Returns the window's SDL_WINDOW_* flags, or 0 if no window is owned.
    SDL_WindowFlags getFlags() const { return SDL_GetWindowFlags(window_); }

    //! Returns the window's properties, or 0 on failure.
    SDL_PropertiesID getProperties() const { return SDL_GetWindowProperties(window_); }

    //! Returns the parent window, or nullptr if there is none.
    //!
    //! @note   The returned pointer is borrowed. Do not call SDL_DestroyWindow on it.
    SDL_Window *getParent() const { return SDL_GetWindowParent(window_); }

    //! Sets or clears the parent window.
    //!
    //! @param	parent	The new parent, or a Window that owns nothing to clear the parent
    //! @warning    SDL destroys this window when it destroys the parent. Destroy or release this Window first.
    bool setParent(Window const &parent) { return SDL_SetWindowParent(window_, parent.get()); }

    //! Sets the title.
    //!
    //! @param	title	The title, in UTF-8
    bool setTitle(char const *title) { return SDL_SetWindowTitle(window_, title); }

    //! Returns the title, in UTF-8, or "" if there is none or no window is owned.
    //!
    //! @note   The returned string belongs to the window and is valid until the title changes.
    char const *getTitle() const { return SDL_GetWindowTitle(window_); }

    //! Sets the icon.
    //!
    //! @param	icon	The icon. SDL copies it, so it can be destroyed afterward.
    bool setIcon(Surface const &icon) { return SDL_SetWindowIcon(window_, icon.get()); }

    //! Requests a new position.
    //!
    //! @param	x,y		The position of the window's client area, or SDL_WINDOWPOS_CENTERED or
    //!					SDL_WINDOWPOS_UNDEFINED
    bool setPosition(int x, int y) { return SDL_SetWindowPosition(window_, x, y); }

    //! Gets the position of the window's client area.
    //!
    //! @param	x,y		Receive the position
    bool getPosition(int &x, int &y) const { return SDL_GetWindowPosition(window_, &x, &y); }

    //! Requests a new size of the client area.
    //!
    //! @param	width,height	The size
    bool setSize(int width, int height) { return SDL_SetWindowSize(window_, width, height); }

    //! Gets the size of the client area.
    //!
    //! @param	width,height	Receive the size
    bool getSize(int &width, int &height) const { return SDL_GetWindowSize(window_, &width, &height); }

    //! Gets the size of the client area in pixels, which may differ from its size on a high-density display.
    //!
    //! @param	width,height	Receive the size
    bool getSizeInPixels(int &width, int &height) const { return SDL_GetWindowSizeInPixels(window_, &width, &height); }

    //! Gets the area of the client area that is safe for interactive content, away from notches and rounded corners.
    //!
    //! @param	rect	Receives the safe area, relative to the client area
    bool getSafeArea(SDL_Rect &rect) const { return SDL_GetWindowSafeArea(window_, &rect); }

    //! Sets the range of aspect ratios (width divided by height) of the client area.
    //!
    //! @param	minAspect	The minimum aspect ratio, or 0 for no minimum
    //! @param	maxAspect	The maximum aspect ratio, or 0 for no maximum
    bool setAspectRatio(float minAspect, float maxAspect)
    {
        return SDL_SetWindowAspectRatio(window_, minAspect, maxAspect);
    }

    //! Gets the range of aspect ratios of the client area.
    //!
    //! @param	minAspect	Receives the minimum aspect ratio, or 0 for no minimum
    //! @param	maxAspect	Receives the maximum aspect ratio, or 0 for no maximum
    bool getAspectRatio(float &minAspect, float &maxAspect) const
    {
        return SDL_GetWindowAspectRatio(window_, &minAspect, &maxAspect);
    }

    //! Gets the size of the window's borders (the decorations) around the client area.
    //!
    //! @param	top,left,bottom,right	Receive the size of each border
    //! @note   This can fail, for example on platforms that don't report it, or before the window is shown.
    bool getBordersSize(int &top, int &left, int &bottom, int &right) const
    {
        return SDL_GetWindowBordersSize(window_, &top, &left, &bottom, &right);
    }

    //! Sets the minimum size of the client area.
    //!
    //! @param	width,height	The minimum size, or 0 for no minimum
    bool setMinimumSize(int width, int height) { return SDL_SetWindowMinimumSize(window_, width, height); }

    //! Gets the minimum size of the client area.
    //!
    //! @param	width,height	Receive the minimum size
    bool getMinimumSize(int &width, int &height) const { return SDL_GetWindowMinimumSize(window_, &width, &height); }

    //! Sets the maximum size of the client area.
    //!
    //! @param	width,height	The maximum size, or 0 for no maximum
    bool setMaximumSize(int width, int height) { return SDL_SetWindowMaximumSize(window_, width, height); }

    //! Gets the maximum size of the client area.
    //!
    //! @param	width,height	Receive the maximum size
    bool getMaximumSize(int &width, int &height) const { return SDL_GetWindowMaximumSize(window_, &width, &height); }

    //! Adds or removes the window's borders (the decorations).
    //!
    //! @param	bordered	true to add the borders
    bool setBordered(bool bordered) { return SDL_SetWindowBordered(window_, bordered); }

    //! Lets or prevents the user resizing the window.
    //!
    //! @param	resizable	true to let the user resize the window
    bool setResizable(bool resizable) { return SDL_SetWindowResizable(window_, resizable); }

    //! Keeps or stops keeping the window above other windows.
    //!
    //! @param	onTop	true to keep the window on top
    bool setAlwaysOnTop(bool onTop) { return SDL_SetWindowAlwaysOnTop(window_, onTop); }

    //! Makes the window fill or stop filling the web page (Emscripten only).
    //!
    //! @param	fill	true to fill the document
    bool setFillDocument(bool fill) { return SDL_SetWindowFillDocument(window_, fill); }

    //! Makes the window modal to its parent, or not.
    //!
    //! @param	modal	true to make the window modal
    //! @note   The window must have a parent (see setParent).
    bool setModal(bool modal) { return SDL_SetWindowModal(window_, modal); }

    //! Lets or prevents the window getting input focus.
    //!
    //! @param	focusable	true to let the window get input focus
    bool setFocusable(bool focusable) { return SDL_SetWindowFocusable(window_, focusable); }

    //! Sets the opacity.
    //!
    //! @param	opacity		The opacity, from 0 (transparent) to 1 (opaque). Values outside the range are clamped.
    bool setOpacity(float opacity) { return SDL_SetWindowOpacity(window_, opacity); }

    //! Returns the opacity, from 0 (transparent) to 1 (opaque), or -1 on failure.
    float getOpacity() const { return SDL_GetWindowOpacity(window_); }

    //! Sets the shape of a window created with SDL_WINDOW_TRANSPARENT. Pixels where the shape's alpha is 0 are
    //! transparent to the user and to the mouse.
    //!
    //! @param	shape	The shape, the size of the window, or a Surface that owns nothing to remove the shape. SDL
    //!					keeps its own reference to it.
    bool setShape(Surface const &shape) { return SDL_SetWindowShape(window_, shape.get()); }

    //! Sets a callback that decides which parts of the window act as a title bar or resize borders.
    //!
    //! @param	callback		The callback, or nullptr to remove it
    //! @param	callbackData	The value passed to the callback
    bool setHitTest(SDL_HitTest callback, void *callbackData)
    {
        return SDL_SetWindowHitTest(window_, callback, callbackData);
    }

    //! Sets the progress state shown on the window's taskbar button.
    //!
    //! @param	state	The progress state
    bool setProgressState(SDL_ProgressState state) { return SDL_SetWindowProgressState(window_, state); }

    //! Returns the progress state shown on the window's taskbar button, or SDL_PROGRESS_STATE_INVALID on failure.
    SDL_ProgressState getProgressState() const { return SDL_GetWindowProgressState(window_); }

    //! Sets the progress value shown on the window's taskbar button.
    //!
    //! @param	value	The progress, from 0 to 1
    bool setProgressValue(float value) { return SDL_SetWindowProgressValue(window_, value); }

    //! Returns the progress value shown on the window's taskbar button, or -1 on failure.
    float getProgressValue() const { return SDL_GetWindowProgressValue(window_); }

    //! @}

    //! @name Window state
    //! These wrap the SDL functions that show, hide, and arrange a window. They return false on failure (for example,
    //! when no window is owned); call SDL_GetError for the reason. Most are requests that the window system may carry
    //! out later; call sync to wait for them.
    //! @{

    //! Shows the window.
    bool show() { return SDL_ShowWindow(window_); }

    //! Hides the window.
    bool hide() { return SDL_HideWindow(window_); }

    //! Raises the window above other windows and asks for input focus.
    bool raise() { return SDL_RaiseWindow(window_); }

    //! Maximizes the window. Nothing happens if the window isn't resizable.
    bool maximize() { return SDL_MaximizeWindow(window_); }

    //! Minimizes the window.
    bool minimize() { return SDL_MinimizeWindow(window_); }

    //! Restores the size and position of a minimized or maximized window.
    bool restore() { return SDL_RestoreWindow(window_); }

    //! Enters or leaves fullscreen.
    //!
    //! @param	fullscreen	true to enter fullscreen
    //! @note   The fullscreen mode is the one set with setFullscreenMode, or borderless fullscreen desktop if none
    //!         is set.
    bool setFullscreen(bool fullscreen) { return SDL_SetWindowFullscreen(window_, fullscreen); }

    //! Waits for the window system to carry out the requested changes to the window's state, position, and size.
    //!
    //! @return false if the changes weren't all carried out before a timeout (see SDL_HINT_VIDEO_SYNC_WINDOW_OPERATIONS)
    bool sync() { return SDL_SyncWindow(window_); }

    //! Requests that the user's attention be drawn to the window, for example by flashing its taskbar button.
    //!
    //! @param	operation	How to flash the window
    bool flash(SDL_FlashOperation operation) { return SDL_FlashWindow(window_, operation); }

    //! Shows the system's window menu at a location.
    //!
    //! @param	x,y		The location, relative to the client area
    bool showSystemMenu(int x, int y) { return SDL_ShowWindowSystemMenu(window_, x, y); }

    //! @}

    //! @name Input grabs
    //! These wrap the SDL functions that confine the keyboard and mouse to a window. A function that returns bool
    //! returns false on failure (for example, when no window is owned); call SDL_GetError for the reason.
    //! @{

    //! Grabs or releases the keyboard, so that system shortcuts such as Alt+Tab go to the window.
    //!
    //! @param	grabbed		true to grab the keyboard
    //! @note   The keyboard is grabbed only while the window has input focus.
    bool setKeyboardGrab(bool grabbed) { return SDL_SetWindowKeyboardGrab(window_, grabbed); }

    //! Returns true if the window has grabbed the keyboard.
    bool getKeyboardGrab() const { return SDL_GetWindowKeyboardGrab(window_); }

    //! Grabs or releases the mouse, confining it to the window.
    //!
    //! @param	grabbed		true to grab the mouse
    bool setMouseGrab(bool grabbed) { return SDL_SetWindowMouseGrab(window_, grabbed); }

    //! Returns true if the window has grabbed the mouse.
    bool getMouseGrab() const { return SDL_GetWindowMouseGrab(window_); }

    //! Confines the mouse to a rectangle in the window while the window has input focus.
    //!
    //! @param	rect	The rectangle, relative to the client area, or nullptr to remove the confinement
    bool setMouseRect(SDL_Rect const *rect) { return SDL_SetWindowMouseRect(window_, rect); }

    //! Returns the rectangle the mouse is confined to, or nullptr if there is none.
    //!
    //! @note   The returned rectangle belongs to the window.
    SDL_Rect const *getMouseRect() const { return SDL_GetWindowMouseRect(window_); }

    //! @}

    //! @name Display
    //! These wrap the SDL functions that relate a window to its display. A function that returns bool returns false
    //! on failure (for example, when no window is owned); call SDL_GetError for the reason.
    //! @{

    //! Returns the display that contains the center of the window, or 0 on failure.
    SDL_DisplayID getDisplay() const { return SDL_GetDisplayForWindow(window_); }

    //! Returns the number of pixels per screen coordinate, or 0 on failure.
    float getPixelDensity() const { return SDL_GetWindowPixelDensity(window_); }

    //! Returns the scale at which the window's content should be drawn, or 0 on failure. It combines the pixel
    //! density and the display's content scale.
    float getDisplayScale() const { return SDL_GetWindowDisplayScale(window_); }

    //! Returns the pixel format of the window, or SDL_PIXELFORMAT_UNKNOWN on failure.
    SDL_PixelFormat getPixelFormat() const { return SDL_GetWindowPixelFormat(window_); }

    //! Sets the display mode used when the window is fullscreen.
    //!
    //! @param	mode	A mode from SDL_GetFullscreenDisplayModes or SDL_GetClosestFullscreenDisplayMode, or nullptr
    //!					for borderless fullscreen desktop
    bool setFullscreenMode(SDL_DisplayMode const *mode) { return SDL_SetWindowFullscreenMode(window_, mode); }

    //! Returns the display mode used when the window is fullscreen, or nullptr for borderless fullscreen desktop.
    //!
    //! @note   The returned mode belongs to SDL.
    SDL_DisplayMode const *getFullscreenMode() const { return SDL_GetWindowFullscreenMode(window_); }

    //! Returns the ICC profile of the window's display, or an empty vector on failure.
    std::vector<Uint8> getICCProfile() const;

    //! @}

    //! @name Window surface
    //! These wrap the SDL functions for drawing to the window through a surface, without a renderer. Don't use them
    //! with an SDL_Renderer for the same window. A function that returns bool returns false on failure (for example,
    //! when no window is owned); call SDL_GetError for the reason.
    //! @{

    //! Returns the surface to draw to, creating it if necessary, or nullptr on failure. Call updateSurface to show
    //! what was drawn.
    //!
    //! @warning    The returned surface belongs to the window. Do not call SDL_DestroySurface on it, and don't wrap it
    //!             in a Surface. It becomes invalid when the window is resized or destroyed, or when destroySurface
    //!             is called. Call getSurface again after that.
    SDL_Surface *getSurface() const { return SDL_GetWindowSurface(window_); }

    //! Returns true if the window has a surface.
    bool hasSurface() const { return SDL_WindowHasSurface(window_); }

    //! Copies the window's surface to the screen.
    bool updateSurface() { return SDL_UpdateWindowSurface(window_); }

    //! Copies rectangles of the window's surface to the screen.
    //!
    //! @param	rects	The rectangles to copy
    //! @param	count	The number of rectangles
    bool updateSurfaceRects(SDL_Rect const *rects, int count)
    {
        return SDL_UpdateWindowSurfaceRects(window_, rects, count);
    }

    //! Destroys the window's surface.
    bool destroySurface() { return SDL_DestroyWindowSurface(window_); }

    //! Sets the vertical sync of updateSurface.
    //!
    //! @param	vsync	The number of vertical refreshes per update (1 to sync with every refresh),
    //!					SDL_WINDOW_SURFACE_VSYNC_DISABLED, or SDL_WINDOW_SURFACE_VSYNC_ADAPTIVE
    bool setSurfaceVSync(int vsync) { return SDL_SetWindowSurfaceVSync(window_, vsync); }

    //! Gets the vertical sync of updateSurface.
    //!
    //! @param	vsync	Receives the vertical sync (see setSurfaceVSync)
    bool getSurfaceVSync(int &vsync) const { return SDL_GetWindowSurfaceVSync(window_, &vsync); }

    //! @}

  private:
    // Destroys a window, if there is one. SDL_DestroyWindow(nullptr) would set SDL's error message.
    static void destroy(SDL_Window *window) noexcept;

    SDL_Window *window_ = nullptr; // The owned window
};

} // namespace Sdlx
