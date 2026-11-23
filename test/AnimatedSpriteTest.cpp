// Tests for Sdlx::Sprite (data/animation behavior) and Sdlx::AnimatedSprite.

#include <Sdlx/Sprite.h>

#include <gtest/gtest.h>

using Sdlx::AnimatedSprite;
using Animation = AnimatedSprite::Animation;
using AnimationGroup = AnimatedSprite::AnimationGroup;

namespace
{

// Three images on a sheet, each 10x10, with distinct offsets so the current
// image can be identified. Animations (all three frames last one second each):
//   0: loop, 1: once, 2: ping-pong
class AnimatedSpriteTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        for (int i = 0; i < 3; ++i)
        {
            group.images.push_back({SDL_Rect{i * 10, 0, 10, 10}, i, i * 2});
        }

        Animation animation;
        animation.frames = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};

        animation.mode = Animation::MODE_LOOP;
        group.animations.push_back(animation);
        animation.mode = Animation::MODE_ONCE;
        group.animations.push_back(animation);
        animation.mode = Animation::MODE_PINGPONG;
        group.animations.push_back(animation);
    }

    static constexpr int LOOP = 0;
    static constexpr int ONCE = 1;
    static constexpr int PINGPONG = 2;

    AnimationGroup group;
};

} // anonymous namespace

TEST(SpriteTest, DefaultConstructorZeroInitializes)
{
    Sdlx::Sprite sprite;

    EXPECT_EQ(sprite.x_, 0.0f);
    EXPECT_EQ(sprite.y_, 0.0f);
    EXPECT_EQ(sprite.rect_.x, 0);
    EXPECT_EQ(sprite.rect_.y, 0);
    EXPECT_EQ(sprite.rect_.w, 0);
    EXPECT_EQ(sprite.rect_.h, 0);
    EXPECT_EQ(sprite.offsetX_, 0);
    EXPECT_EQ(sprite.offsetY_, 0);
}

TEST(SpriteTest, ConstructorStoresArguments)
{
    Sdlx::Sprite sprite(nullptr, SDL_Rect{1, 2, 3, 4}, 5, 6, 7.5f, 8.5f);

    EXPECT_EQ(sprite.rect_.x, 1);
    EXPECT_EQ(sprite.rect_.y, 2);
    EXPECT_EQ(sprite.rect_.w, 3);
    EXPECT_EQ(sprite.rect_.h, 4);
    EXPECT_EQ(sprite.offsetX_, 5);
    EXPECT_EQ(sprite.offsetY_, 6);
    EXPECT_EQ(sprite.x_, 7.5f);
    EXPECT_EQ(sprite.y_, 8.5f);
}

TEST_F(AnimatedSpriteTest, ConstructorSetsPosition)
{
    AnimatedSprite sprite(nullptr, &group, 3.0f, 4.0f);

    EXPECT_EQ(sprite.x_, 3.0f);
    EXPECT_EQ(sprite.y_, 4.0f);
    EXPECT_EQ(sprite.GetAnimation(), 0);
    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.GetTime(), 0.0f);
}

TEST_F(AnimatedSpriteTest, ConstructorSelectsFirstFrameOfFirstAnimation)
{
    // Make the first frame something other than image 0, whose values are mostly zero
    group.animations[0].frames[0].index = 2;

    AnimatedSprite sprite(nullptr, &group);

    EXPECT_EQ(sprite.rect_.x, 20);
    EXPECT_EQ(sprite.rect_.w, 10);
    EXPECT_EQ(sprite.rect_.h, 10);
    EXPECT_EQ(sprite.offsetX_, 2);
    EXPECT_EQ(sprite.offsetY_, 4);
}

TEST(AnimatedSpriteEmptyGroupTest, SpriteHasNoImageAndAnimationFunctionsDoNothing)
{
    AnimationGroup empty;
    AnimatedSprite sprite(nullptr, &empty);

    sprite.AdvanceTime(1.0f);
    sprite.Service(1.0f);
    sprite.SetTime(2.0f);
    sprite.SetFrame(1);

    EXPECT_EQ(sprite.GetAnimation(), 0);
    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.GetTime(), 0.0f);
    EXPECT_EQ(sprite.rect_.w, 0);
    EXPECT_EQ(sprite.rect_.h, 0);
}

TEST_F(AnimatedSpriteTest, AnimationWithNoFramesHasNoImage)
{
    for (int mode : {Animation::MODE_ONCE, Animation::MODE_LOOP, Animation::MODE_PINGPONG})
    {
        Animation none;
        none.mode = Animation::Mode(mode);
        group.animations.push_back(none);
        int const index = int(group.animations.size()) - 1;

        AnimatedSprite sprite(nullptr, &group);
        sprite.PlayAnimation(index, AnimatedSprite::DIR_BACKWARD);
        EXPECT_EQ(sprite.GetAnimation(), index);
        EXPECT_EQ(sprite.GetFrame(), 0);
        EXPECT_EQ(sprite.rect_.w, 0);

        sprite.AdvanceTime(1.5f); // must not read a frame
        EXPECT_EQ(sprite.GetFrame(), 0);
        EXPECT_FLOAT_EQ(sprite.GetTime(), 1.5f);

        sprite.SetTime(2.5f);
        EXPECT_EQ(sprite.GetFrame(), 0);
        EXPECT_FLOAT_EQ(sprite.GetTime(), 2.5f);
        EXPECT_EQ(sprite.rect_.w, 0);
    }
}

TEST(AnimationGroupTest, MembersHaveDefaultValues)
{
    Animation animation;
    EXPECT_EQ(animation.mode, Animation::MODE_ONCE);
    EXPECT_TRUE(animation.frames.empty());

    AnimatedSprite::Frame frame;
    EXPECT_EQ(frame.index, 0);
    EXPECT_EQ(frame.time, 0.0f);

    AnimationGroup::Image image;
    EXPECT_EQ(image.rect.x, 0);
    EXPECT_EQ(image.rect.y, 0);
    EXPECT_EQ(image.rect.w, 0);
    EXPECT_EQ(image.rect.h, 0);
    EXPECT_EQ(image.offsetX, 0);
    EXPECT_EQ(image.offsetY, 0);
}

TEST_F(AnimatedSpriteTest, PlayAnimationForwardStartsAtFirstFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);

    EXPECT_EQ(sprite.GetAnimation(), LOOP);
    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.GetTime(), 0.0f);
    EXPECT_EQ(sprite.rect_.x, 0);
    EXPECT_EQ(sprite.offsetX_, 0);
    EXPECT_EQ(sprite.offsetY_, 0);
}

TEST_F(AnimatedSpriteTest, PlayAnimationBackwardStartsAtLastFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP, AnimatedSprite::DIR_BACKWARD);

    EXPECT_EQ(sprite.GetFrame(), 2);
    EXPECT_EQ(sprite.rect_.x, 20);
    EXPECT_EQ(sprite.offsetX_, 2);
    EXPECT_EQ(sprite.offsetY_, 4);
}

TEST_F(AnimatedSpriteTest, AdvanceWithinFrameDoesNotChangeFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(0.5f);

    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 0.5f);
}

TEST_F(AnimatedSpriteTest, AdvanceZeroDoesNothing)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(0.0f);

    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.GetTime(), 0.0f);
}

TEST_F(AnimatedSpriteTest, AdvanceCrossesFramesAndUpdatesImage)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(1.5f);

    EXPECT_EQ(sprite.GetFrame(), 1);
    EXPECT_EQ(sprite.rect_.x, 10);
    EXPECT_EQ(sprite.offsetX_, 1);
    EXPECT_EQ(sprite.offsetY_, 2);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 1.5f);
}

TEST_F(AnimatedSpriteTest, AdvanceExactlyToFrameBoundary)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 1);

    // The time spent in the new frame must be zero, so half a second more stays
    // in the same frame.
    sprite.AdvanceTime(0.5f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    sprite.AdvanceTime(0.5f);
    EXPECT_EQ(sprite.GetFrame(), 2);
}

TEST_F(AnimatedSpriteTest, PartialAdvancesAccumulate)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);

    sprite.AdvanceTime(0.75f);
    EXPECT_EQ(sprite.GetFrame(), 0);
    sprite.AdvanceTime(0.75f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 1.5f);
}

TEST_F(AnimatedSpriteTest, LoopWrapsToFirstFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(3.5f);

    EXPECT_EQ(sprite.GetFrame(), 0);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 1);
}

TEST_F(AnimatedSpriteTest, BackwardLoopWrapsToLastFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP, AnimatedSprite::DIR_BACKWARD);

    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 0);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 2);
}

TEST_F(AnimatedSpriteTest, OnceFreezesOnLastFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(ONCE);

    sprite.AdvanceTime(2.5f);
    EXPECT_EQ(sprite.GetFrame(), 2);

    sprite.AdvanceTime(100.0f);
    EXPECT_EQ(sprite.GetFrame(), 2);
    EXPECT_EQ(sprite.rect_.x, 20);
}

TEST_F(AnimatedSpriteTest, PingPongReversesAtTheEnds)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(PINGPONG);

    sprite.AdvanceTime(2.0f);
    EXPECT_EQ(sprite.GetFrame(), 2);

    // The last frame plays once more as the direction reverses, then the
    // animation steps backward.
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 2);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 0);

    // ...and reverses again at the start.
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 0);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 1);
}

TEST_F(AnimatedSpriteTest, SetTimeJumpsToAbsoluteTime)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(2.5f);

    sprite.SetTime(1.25f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 1.25f);
}

TEST_F(AnimatedSpriteTest, SetFrameSetsFrameAndTime)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);

    sprite.SetFrame(2);
    EXPECT_EQ(sprite.GetFrame(), 2);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 2.0f);
    EXPECT_EQ(sprite.rect_.x, 20);
    EXPECT_EQ(sprite.offsetY_, 4);

    // Playback continues from the start of the selected frame.
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 0);
}

TEST_F(AnimatedSpriteTest, SetFrameBackwardCountsFramesAfterIt)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP, AnimatedSprite::DIR_BACKWARD);

    sprite.SetFrame(0); // must terminate
    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 2.0f);
}

TEST_F(AnimatedSpriteTest, ServiceAdvancesTime)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.Service(1.25f);

    EXPECT_EQ(sprite.GetFrame(), 1);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 1.25f);
}

TEST_F(AnimatedSpriteTest, LoopWithZeroDurationFramesStaysOnCurrentFrame)
{
    Animation still;
    still.mode = Animation::MODE_LOOP;
    still.frames = {{1, 0.0f}};
    group.animations.push_back(still);

    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(3);
    sprite.Service(0.016f); // must terminate

    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.rect_.x, 10);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 0.016f);
}

TEST_F(AnimatedSpriteTest, PingPongWithZeroDurationFramesStaysOnCurrentFrame)
{
    Animation still;
    still.mode = Animation::MODE_PINGPONG;
    still.frames = {{0, 0.0f}, {2, 0.0f}};
    group.animations.push_back(still);

    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(3);
    sprite.AdvanceTime(5.0f); // must terminate

    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.rect_.x, 0);
    EXPECT_FLOAT_EQ(sprite.GetTime(), 5.0f);
}

// Subtracting a frame's duration from a float as large as 1e9 does not change
// it, so these would never return if the time were used up one frame at a time.

TEST_F(AnimatedSpriteTest, VeryLargeAdvanceOfLoopLandsOnTheRightFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);

    // 333,333,333 whole loops of 3 seconds, and 1 second more
    sprite.AdvanceTime(1.0e9f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 2);
}

TEST_F(AnimatedSpriteTest, VeryLargeAdvanceOfPingPongLandsOnTheRightFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(PINGPONG);

    // 166,666,666 whole round trips of 6 seconds, and 4 seconds more. A round
    // trip is frames 0 1 2 2 1 0, so the animation is on frame 1, going backward.
    sprite.AdvanceTime(1.0e9f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    sprite.AdvanceTime(1.0f);
    EXPECT_EQ(sprite.GetFrame(), 0);
}

TEST_F(AnimatedSpriteTest, AdvancingByWholeLoopsKeepsThePositionInTheFrame)
{
    AnimatedSprite sprite(nullptr, &group);
    sprite.PlayAnimation(LOOP);
    sprite.AdvanceTime(1.5f);

    sprite.AdvanceTime(3000.0f);
    EXPECT_EQ(sprite.GetFrame(), 1);

    // Still half a second into frame 1
    sprite.AdvanceTime(0.25f);
    EXPECT_EQ(sprite.GetFrame(), 1);
    sprite.AdvanceTime(0.25f);
    EXPECT_EQ(sprite.GetFrame(), 2);
}

// A default-constructed sprite has no animation group. Its animation functions
// assert in a debug build, so the release behavior (do nothing) can be tested
// only when asserts are disabled.

#if defined(NDEBUG)

TEST(AnimatedSpriteWithoutGroupTest, AnimationFunctionsDoNothing)
{
    AnimatedSprite sprite;

    sprite.PlayAnimation(1);
    sprite.AdvanceTime(1.0f);
    sprite.Service(1.0f);
    sprite.SetTime(2.0f);
    sprite.SetFrame(1);

    EXPECT_EQ(sprite.GetAnimation(), 0);
    EXPECT_EQ(sprite.GetFrame(), 0);
    EXPECT_EQ(sprite.GetTime(), 0.0f);
    EXPECT_EQ(sprite.rect_.w, 0);
    EXPECT_EQ(sprite.rect_.h, 0);
}

#endif // defined(NDEBUG)

TEST_F(AnimatedSpriteTest, AnimationGroupIsSharedBetweenSprites)
{
    AnimatedSprite a(nullptr, &group);
    AnimatedSprite b(nullptr, &group);
    a.PlayAnimation(LOOP);
    b.PlayAnimation(LOOP);

    a.AdvanceTime(1.0f);
    EXPECT_EQ(a.GetFrame(), 1);
    EXPECT_EQ(b.GetFrame(), 0);
}

TEST(AnimationGroupTest, ConstructorCopiesLists)
{
    AnimationGroup::ImageList images = {{SDL_Rect{0, 0, 1, 1}, 0, 0}};
    AnimationGroup::AnimationList animations(2);

    AnimationGroup group(images, animations);

    EXPECT_EQ(group.images.size(), 1u);
    EXPECT_EQ(group.animations.size(), 2u);
}
