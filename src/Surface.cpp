#include "Surface.h"

#include <cassert>
#include <utility>

namespace Sdlx
{

// If both Surfaces already share the SDL_Surface, each already holds a reference, so nothing changes.
Surface &Surface::operator=(Surface const &other) noexcept
{
    if (other.surface_ != surface_)
    {
        if (other.surface_ != nullptr)
        {
            ++other.surface_->refcount;
        }
        SDL_Surface *old = surface_;
        surface_ = other.surface_;
        SDL_DestroySurface(old);
    }
    return *this;
}

// This doesn't use reset, because reset does nothing when given the surface it already holds. If both Surfaces
// share the SDL_Surface, the other Surface's reference must still be released.
Surface &Surface::operator=(Surface &&other) noexcept
{
    if (this != &other)
    {
        SDL_Surface *old = surface_;
        surface_ = other.release();
        SDL_DestroySurface(old);
    }
    return *this;
}

SDL_Surface *Surface::release() noexcept
{
    SDL_Surface *surface = surface_;
    surface_ = nullptr;
    return surface;
}

void Surface::reset(SDL_Surface *surface /* = nullptr */) noexcept
{
    if (surface != surface_)
    {
        SDL_Surface *old = surface_;
        surface_ = surface;
        SDL_DestroySurface(old);
    }
}

void Surface::swap(Surface &other) noexcept
{
    std::swap(surface_, other.surface_);
}

SDL_Surface *Surface::operator->() const noexcept
{
    assert(surface_ != nullptr);
    return surface_;
}

std::vector<SDL_Surface *> Surface::getImages() const
{
    std::vector<SDL_Surface *> images;
    int count = 0;
    SDL_Surface **array = SDL_GetSurfaceImages(surface_, &count);
    if (array != nullptr)
    {
        images.assign(array, array + count);
        SDL_free(array);
    }
    return images;
}

// The decision to unlock is made here and kept in locked_. The destructor can't use SDL_MUSTLOCK, because SDL sets
// SDL_SURFACE_LOCK_NEEDED while a surface is locked, so it is always true then.
Surface::Lock::Lock(Surface &surface) : surface_(surface.get()), locked_(false)
{
    if (surface_ != nullptr && SDL_MUSTLOCK(surface_))
    {
        locked_ = SDL_LockSurface(surface_);
        if (!locked_)
        {
            surface_ = nullptr;
        }
    }
}

Surface::Lock::~Lock()
{
    if (locked_)
    {
        SDL_UnlockSurface(surface_);
    }
}

} // namespace Sdlx
