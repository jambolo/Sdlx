#include "Window.h"

#include <utility>

namespace Sdlx
{

// Self-move leaves the window alone: release returns the owned window, and reset does nothing when given the window
// it already holds.
Window &Window::operator=(Window &&other) noexcept
{
    reset(other.release());
    return *this;
}

SDL_Window *Window::release() noexcept
{
    SDL_Window *window = window_;
    window_ = nullptr;
    return window;
}

void Window::reset(SDL_Window *window /* = nullptr */) noexcept
{
    if (window != window_)
    {
        SDL_Window *old = window_;
        window_ = window;
        destroy(old);
    }
}

void Window::swap(Window &other) noexcept
{
    std::swap(window_, other.window_);
}

// SDL_CreatePopupWindow doesn't check the parent before it initializes the video subsystem if necessary, so a Window
// that owns nothing would initialize it.
Window Window::createPopup(int offsetX, int offsetY, int width, int height, SDL_WindowFlags flags) const
{
    if (window_ == nullptr)
    {
        SDL_InvalidParamError("parent");
        return Window();
    }
    return Window(SDL_CreatePopupWindow(window_, offsetX, offsetY, width, height, flags));
}

// SDL_GetWindowICCProfile checks neither the window nor the video subsystem, so it crashes on nullptr.
std::vector<Uint8> Window::getICCProfile() const
{
    std::vector<Uint8> profile;
    if (window_ == nullptr)
    {
        SDL_InvalidParamError("window");
        return profile;
    }
    std::size_t size = 0;
    void *data = SDL_GetWindowICCProfile(window_, &size);
    if (data != nullptr)
    {
        Uint8 const *bytes = static_cast<Uint8 const *>(data);
        profile.assign(bytes, bytes + size);
        SDL_free(data);
    }
    return profile;
}

void Window::destroy(SDL_Window *window) noexcept
{
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
    }
}

} // namespace Sdlx
