#pragma once

#include <SDL3/SDL.h>

//! SDL3 extension library.
namespace Sdlx
{

//! Loads an image file (using SDL3_image). Free the result with SDL_DestroySurface.
SDL_Surface *LoadSurface(char const *filename);

//! Loads an image file and applies a color key. A paletted image is converted to 32-bit RGBA.
SDL_Surface *LoadColorKeyedSurface(char const *filename, SDL_Color key);

//! Idle processing callback.
typedef bool (*EventLoopIdleCallback)();

//! Event loop event handler
typedef bool (*EventLoopEventHandler)(SDL_Event const &event);

//! Handle events until a SDL_EVENT_QUIT event is received. Returns false if the SDL events subsystem is not
//! initialized.
bool EventLoop(EventLoopEventHandler pEH = 0, EventLoopIdleCallback pIdle = 0);

//! Returns an opaque color with the specified values
[[deprecated("Use SDL_Color{ r, g, b, 255 } instead")]]
SDL_Color MakeColor(int red, int green, int blue);

//! Returns a rect with the specified values
[[deprecated("Use SDL_Rect{ x, y, w, h } instead")]]
SDL_Rect MakeRect(int x, int y, int w, int h);

} // namespace Sdlx
