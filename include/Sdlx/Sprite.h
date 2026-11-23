/** @file *//********************************************************************************************************

                                                       Sprite.h

						                    Copyright 2006, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/Sdlx/Sprite.h#1 $

	$NoKeywords: $

 ********************************************************************************************************************/

#pragma once


#include "Sdlx.h"

#include <SDL.h>

#include <vector>

namespace Sdlx
{

class SpriteAnimationGroup;

/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! A sprite
//
//! A sprite is a 2D rectangular image that has a location on the display. The image is generally implemented as a
//! sub-region of a "sheet". The sprite also has an origin specified as an offset from the UL corner.

class Sprite
{
public:
	//! Default constructor
	Sprite();

	//! Constructor
	Sprite( SDL_Surface *		sheet,
			SDL_Rect const &	rect = MakeRect( 0, 0, 0, 0 ),
			int					offsetX = 0,
			int					offsetY = 0,
			float				x = 0,
			float				y = 0 );

	// Destructor
	virtual ~Sprite();

	//! Draws the sprite
	void Draw( SDL_Surface * dst ) const;

	float		m_x;		//!< Location of the sprite's origin on the display
	float		m_y;		//!< Location of the sprite's origin on the display
	SDL_Rect	m_rect;		//!< Location and size of the sprite in the image
	int			m_offsetX;	//!< Offset from the the UL corner to the sprite's origin
	int			m_offsetY;	//!< Offset from the the UL corner to the sprite's origin

private:

	SDL_Surface	*	m_sheet;	// The sheet containing the sprite's image
};


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! An animated sprite
//
//! This class is a sprite whose image is animated. Animations are basically a list of frames on a sheet and the
//! durations for each frame. An animated sprite can have several different animations, though all frames of the
//! animation must come from a single sheet. The animations can stop on the last frame, can loop, or can play back
//! and forth ( "ping-pong" ).
//!
//! Animation group
//!
//! An animation "group" consists of a list of animations and a list of all the images to used in the animations.
//! The images are all sub-regions of a single sheet. The animations reference the list of images, and an image
//! can be used in several animations and even multiple times in a single animation.
//!
//! Animation
//!
//! An animation is simply a list of frames and a mode that determines what to do when the end of the list of
//! frames is reached. Each frame contains the index of its image (in the image list) and its
//! duration.

class AnimatedSprite : public Sprite
{
	friend class SpriteFactory;

public:

	//! Direction of playback
	enum Direction
	{
		DIR_FORWARD,		//!< Play the animation forward
		DIR_BACKWARD		//!< Play the animation backward
	};

	//! A frame of an animation
	struct Frame
	{
		int		index;	//!< The index of the image in the image list
		float	time;	//!< The duration of the frame
	};

	//! An animation
	struct Animation
	{
		//! A vector of animation frames
		typedef std::vector< Frame >	FrameList;

		//! The behavior of an animation when it reaches the end of the animation
		enum Mode
		{
			MODE_ONCE,			//!< Freeze on the last frame
			MODE_LOOP,			//!< Loop forever
			MODE_PINGPONG		//!< When the beginning or end is reached, reverse direction
		};

		Mode		mode;		//!< Looping behavior
		FrameList	frames;		//!< The frames of the animation
	};

	//! A set of animations using a common sheet
	struct AnimationGroup
	{
		//! A frame of an animation
		struct Image
		{
			SDL_Rect	rect;				//!< Location and size within the sheet
			int			offsetX, offsetY;	//!< Offset to the sprite's origin from the UL corner of the sprite
		};

		typedef std::vector< Image >		ImageList;		//!< A vector of images
		typedef std::vector< Animation >	AnimationList;	//!< A vector of animations

		//! Default constructor
		AnimationGroup() {}

		//! Constructor
		AnimationGroup( ImageList const & images, AnimationList const & animations );

		ImageList		images;			//!< All the images used in the group
		AnimationList	animations;		//!< All the animations in this group
	};

	//! Default constructor
	AnimatedSprite();

	//! Constructor
	AnimatedSprite( SDL_Surface * sheet, AnimationGroup const * animations, float x = 0, float y = 0 );

	// Destructor
	virtual ~AnimatedSprite();

	//! Starts an animation
	void PlayAnimation( int index, Direction direction = DIR_FORWARD );

	//! Advances the time (in seconds)
	void AdvanceTime( float elapsed );

	//! Sets the time (in seconds)
	void SetTime( float time );

	//! Sets the current animation frame
	void SetFrame( int intex );

	//! Returns the current animation index
	int GetAnimation() const					{ return m_currentAnimation; }

	//! Returns the current time (in seconds)
	float GetTime() const						{ return m_time; }

	//! Returns the current animation frame
	int GetFrame() const						{ return m_currentFrame; }

	//! Updates the state of the sprite animation
	void Service( float elapsedTime );

private:

	AnimationGroup const *	m_pAnimations;			// The animation group
	int						m_currentAnimation;		// The index of the current animation
	int						m_currentFrame;			// The index of the current frame
	float					m_time;					// The current position of the animation
	float					m_frameTime;			// The time from the start of the current frame
	Direction				m_direction;			// The direction that the animation is playing
};


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

class SpriteFactory
{
	Sprite * LoadSprite( char const * filename );
	AnimatedSprite * LoadAnimatedSprite( char const * filename );
};

} // namespace Sdlx
