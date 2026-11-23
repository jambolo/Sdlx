#include "Sprite.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Sdlx
{

Sprite::Sprite() : x_(0), y_(0), rect_(SDL_Rect{0, 0, 0, 0}), offsetX_(0), offsetY_(0), sheet_(0)
{
}

//! @param	sheet				SDL_Surface containing the sprite's image
//! @param	rect				The location and size of the sprite's image on the sheet
//! @param	offsetX,offsetY		Offset to the origin of the sprite
//! @param	x,y					Initial location of the sprite

Sprite::Sprite(SDL_Surface *sheet, SDL_Rect const &rect /* = SDL_Rect{0, 0, 0, 0}*/, int offsetX /* = 0*/,
               int offsetY /* = 0*/, float x /* = 0*/, float y /* = 0*/)
    : x_(x), y_(y), rect_(rect), offsetX_(offsetX), offsetY_(offsetY), sheet_(sheet)
{
}

//! The position is rounded to the nearest pixel.
//!
//! @param	dst		destination surface

void Sprite::Draw(SDL_Surface *dst) const
{
    bool ok;

    // Use floor, not a cast, so that negative positions also round to the nearest pixel.
    SDL_Rect position = {int(std::floor(x_ - offsetX_ + 0.5f)), int(std::floor(y_ - offsetY_ + 0.5f)), 0, 0};
    SDL_Rect const *pRect = (rect_.w > 0 && rect_.h > 0) ? &rect_ : NULL;

    ok = SDL_BlitSurface(sheet_, pRect, dst, &position);
    assert(ok);
    (void)ok;
}

AnimatedSprite::AnimatedSprite()
    : pAnimations_(0), currentAnimation_(0), currentFrame_(0), time_(0.0f), frameTime_(0.0f), direction_(DIR_FORWARD)
{
}

//!
//! @param	sheet			SDL_Surface containing the sprite's
//! animation frames
//! @param	animations		Description of the sprite's animations
//! @param	x,y				Initial location of the sprite
//!
//! @note	The sprite does not assume ownership of the animation group, so
//! it can be shared by many sprites.
//! @note	The sprite starts on the first frame of the first animation, as
//! if PlayAnimation( 0 ) had been called.

AnimatedSprite::AnimatedSprite(SDL_Surface *sheet, AnimationGroup const *animations, float x /* = 0*/, float y /* = 0*/)
    : Sprite(sheet, SDL_Rect{0, 0, 0, 0}, 0, 0, x, y), pAnimations_(animations), currentAnimation_(0), currentFrame_(0),
      time_(0.0f), frameTime_(0.0f), direction_(DIR_FORWARD)
{
    assert(animations != 0);

    UpdateImage();
}

//! This function starts a sprite's animation. The initial frame depends on the
//! direction of play.
//!
//! @param	index			Which animation to play
//! @param	direction		Which direction to play the animation
//!
//! @note	This function does nothing if the sprite has no animation group
//! (it was default-constructed) or the group has no such animation.

void AnimatedSprite::PlayAnimation(int index, Direction direction /* = DIR_FORWARD*/)
{
    assert(pAnimations_ != 0);
    if (pAnimations_ == 0)
    {
        return;
    }

    assert(index >= 0 && index < int(pAnimations_->animations.size()));
    if (index < 0 || index >= int(pAnimations_->animations.size()))
    {
        return;
    }

    Animation::FrameList const &frames = pAnimations_->animations[index].frames; // Convenience

    currentAnimation_ = index;
    direction_ = direction;
    currentFrame_ = (direction == DIR_FORWARD || frames.empty()) ? 0 : int(frames.size()) - 1;
    time_ = 0.0f;
    frameTime_ = 0.0f;

    UpdateImage();
}

//! This function sets the animation time to an absolute value and updates the
//! animation to reflect the new time.
//!
//! @param	time	new time
//!
//! @note   If the mode is ping-pong, then the initial direction is assumed to be forward.
//! @note   This function does nothing if the sprite has no animation group  (it was default-constructed) or the group
//!         has no animations.

void AnimatedSprite::SetTime(float time)
{
    assert(pAnimations_ != 0);

    Animation const *pAnimation = CurrentAnimation();
    if (pAnimation == 0)
    {
        return;
    }

    if (pAnimation->mode == Animation::MODE_PINGPONG)
    {
        direction_ = DIR_FORWARD;
    }

    PlayAnimation(currentAnimation_, direction_);
    AdvanceTime(time);
}

//! This function advances the animation by the specified amount of time
//!
//! @param	elapsed		elapsed time (must be finite and >= 0)
//!
//! @note	A looping or ping-pong animation whose frames have no duration
//! stays on its current frame, and so does an animation with no frames.
//! @note	This function does nothing if the sprite has no animation group
//! (it was default-constructed) or the group has no animations.

void AnimatedSprite::AdvanceTime(float elapsed)
{
    assert(pAnimations_ != 0);

    Animation const *pAnimation = CurrentAnimation();
    if (pAnimation == 0)
    {
        return;
    }

    assert(elapsed >= 0.0f && std::isfinite(elapsed));

    Animation const &animation = *pAnimation;              // Convenience
    Animation::FrameList const &frames = animation.frames; // Convenience

    // Nothing to do if no time has passed. This also rejects a NaN or infinite
    // time, which would never be used up by the loop below.

    if (!(elapsed > 0.0f) || !std::isfinite(elapsed))
    {
        return;
    }

    // With no frames, there is nothing to step through

    if (frames.empty())
    {
        time_ += elapsed;
        return;
    }

    float e = elapsed + frameTime_;

    // A looping or ping-pong animation repeats, so only the time within the
    // current repetition matters. Dropping the whole repetitions bounds the
    // loop below, even when the elapsed time is too large for subtracting a
    // frame's duration to change it. A ping-pong animation repeats after playing
    // forward and then backward.

    if (animation.mode != Animation::MODE_ONCE)
    {
        float period = 0.0f;
        for (Frame const &f : frames)
        {
            period += f.time;
        }
        if (animation.mode == Animation::MODE_PINGPONG)
        {
            period *= 2.0f;
        }

        // If the frames have no duration, then time cannot move the animation
        // (and the loop below would never end).

        if (!(period > 0.0f))
        {
            time_ += elapsed;
            return;
        }

        e = std::fmod(elapsed, period) + frameTime_;
    }

    // Set the initial parameters based on the current direction

    int step;
    int first;
    int last;

    if (direction_ == DIR_FORWARD)
    {
        step = +1;
        first = 0;
        last = int(frames.size()) - 1;
    }
    else
    {
        step = -1;
        first = int(frames.size()) - 1;
        last = 0;
    }

    // Step through the animation until the correct frame is reached. When done,
    // 'e' is the time spent in the current frame.

    for (;;)
    {
        float currentFrameTime = frames[currentFrame_].time;

        if (e < currentFrameTime)
        {
            break;
        }

        if (currentFrame_ * step < last * step)
        {
            currentFrame_ += step;
        }
        else if (animation.mode == Animation::MODE_LOOP)
        {
            currentFrame_ = first;
        }
        else if (animation.mode == Animation::MODE_PINGPONG)
        {
            direction_ = (direction_ == DIR_FORWARD) ? DIR_BACKWARD : DIR_FORWARD;
            step = -step;
            std::swap(first, last);
        }
        else
        {
            // MODE_ONCE: freeze at the start of the last frame
            e = 0.0f;
            break;
        }

        e -= currentFrameTime;
    }

    frameTime_ = e;

    UpdateImage();

    // Save the time

    time_ += elapsed;
}

//! This function sets the current animation to a specific frame (as opposed to
//! a specific time). The running time of the animation is then set to
//! correspond to the specified frame.
//!
//! @param	index	Desired frame
//!
//! @note If the mode is ping-pong, then the initial direction is assumed to be
//! forward.
//! @note	This function does nothing if the sprite has no animation group
//! (it was default-constructed), the group has no animations, or the
//! animation has no such frame.

void AnimatedSprite::SetFrame(int index)
{
    assert(pAnimations_ != 0);

    Animation const *pAnimation = CurrentAnimation();
    if (pAnimation == 0)
    {
        return;
    }

    Animation const &animation = *pAnimation;              // Convenience
    Animation::FrameList const &frames = animation.frames; // Convenience

    assert(index >= 0 && index < int(frames.size()));
    if (index < 0 || index >= int(frames.size()))
    {
        return;
    }

    time_ = 0.0f;

    if (animation.mode == Animation::MODE_PINGPONG)
    {
        direction_ = DIR_FORWARD;
    }

    switch (direction_)
    {
    case DIR_FORWARD:
        for (int i = 0; i < index; ++i)
        {
            time_ += frames[i].time;
        }
        break;

    case DIR_BACKWARD:
        for (int i = int(frames.size()) - 1; i > index; --i)
        {
            time_ += frames[i].time;
        }
        break;
    }

    currentFrame_ = index;
    frameTime_ = 0.0f;

    UpdateImage();
}

//! This function will update the animation based on the amount of time that has
//! elapsed since it was last called.
//!
//! @param	elapsedTime		Time elapsed since this function was
//! last called (must be >= 0)

void AnimatedSprite::Service(float elapsedTime)
{
    assert(elapsedTime >= 0.0f);

    AdvanceTime(elapsedTime);
}

//! Unlike a Sprite, an animated sprite with an empty image rectangle draws
//! nothing. That is the case when it has no animation group, or its current
//! animation has no frames.
//!
//! @param	dst		destination surface

void AnimatedSprite::Draw(SDL_Surface *dst) const
{
    if (rect_.w <= 0 || rect_.h <= 0)
    {
        return;
    }

    Sprite::Draw(dst);
}

AnimatedSprite::Animation const *AnimatedSprite::CurrentAnimation() const
{
    if (pAnimations_ == 0 || currentAnimation_ < 0 || currentAnimation_ >= int(pAnimations_->animations.size()))
    {
        return 0;
    }

    return &pAnimations_->animations[currentAnimation_];
}

void AnimatedSprite::UpdateImage()
{
    // If there is no current frame, then there is no image

    rect_ = SDL_Rect{0, 0, 0, 0};
    offsetX_ = 0;
    offsetY_ = 0;

    Animation const *pAnimation = CurrentAnimation();
    if (pAnimation == 0 || currentFrame_ < 0 || currentFrame_ >= int(pAnimation->frames.size()))
    {
        return;
    }

    int const imageIndex = pAnimation->frames[currentFrame_].index;
    AnimationGroup::ImageList const &images = pAnimations_->images; // Convenience

    assert(imageIndex >= 0 && imageIndex < int(images.size()));
    if (imageIndex < 0 || imageIndex >= int(images.size()))
    {
        return;
    }

    AnimationGroup::Image const &image = images[imageIndex]; // Convenience

    rect_ = image.rect;
    offsetX_ = image.offsetX;
    offsetY_ = image.offsetY;
}

//! @param	images			List of images used in the animations
//! @param	animations		List of animations

AnimatedSprite::AnimationGroup::AnimationGroup(ImageList const &images, AnimationList const &animations)
    : images(images), animations(animations)
{
}

//! This function creates a sprite described by the specified file. Ownership of
//! the sprite is passed to the caller. The sprite must be deallocated using
//! delete.
//!
//! @param	filename	name of the file containing the sprite
//! description
//!
//! @return	The new sprite.

Sprite *SpriteFactory::LoadSprite(char const * /*filename*/)
{
    return 0;
}

//! This function creates an animated sprite described by the specified file.
//! Ownership of the sprite is passed to the caller. The sprite must be
//! deallocated using delete.
//!
//! @param	filename	name of the file containing the sprite
//! description
//!
//! @return	The new animated sprite.

AnimatedSprite *SpriteFactory::LoadAnimatedSprite(char const * /*filename*/)
{
    return 0;
}

} // namespace Sdlx
