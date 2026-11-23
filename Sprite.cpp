/** @file *//********************************************************************************************************

                                                      Sprite.cpp

						                    Copyright 2006, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/Sdlx/Sprite.cpp#1 $

	$NoKeywords: $

 ********************************************************************************************************************/

#include "PrecompiledHeaders.h"

#include "Sprite.h"

namespace Sdlx
{


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

Sprite::Sprite()
{
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/



/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! @param	sheet				SDL_Surface containing the sprite's image
//! @param	rect				The location and size of the sprite's image on the sheet
//! @param	offsetX,offsetY		Offset to the origin of the sprite
//! @param	x,y					Initial location of the sprite

Sprite::Sprite( SDL_Surface *		sheet,
				SDL_Rect const &	rect/* = MakeRect( 0, 0, 0, 0 )*/,
				int					offsetX/* = 0*/,
				int					offsetY/* = 0*/,
				float				x/* = 0*/,
				float				y/* = 0*/ )
	:	m_sheet( sheet ),
		m_rect( rect ),
		m_offsetX( offsetX ),
		m_offsetY( offsetY ),
		m_x( x ),
		m_y( y )
{
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

Sprite::~Sprite()
{
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//!
//! @param	dst		destination surface

void Sprite::Draw( SDL_Surface * dst ) const
{
	int		rv;

	SDL_Rect			position	= { int( m_x - m_offsetX + 0.5f ), int( m_y - m_offsetY + 0.5f ), 0, 0 };
	SDL_Rect const *	pRect		= ( m_rect.w > 0 && m_rect.h > 0 ) ? &m_rect : NULL;

	rv = SDL_BlitSurface( m_sheet, const_cast< SDL_Rect * >( pRect ), dst, &position );
	assert( rv == 0 );
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

AnimatedSprite::AnimatedSprite()
	:	m_pAnimations( 0 ),
		m_currentAnimation( 0 ),
		m_currentFrame( 0 ),
		m_time( 0.0f ),
		m_frameTime( 0.0f ),
		m_direction( DIR_FORWARD )
{
}




/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//!
//! @param	sheet			SDL_Surface containing the sprite's animation frames
//! @param	animations		Description of the sprite's animations
//! @param	x,y				Initial location of the sprite
//!
//! @note	The sprite does not assume ownership of the animation group, so it can be shared by many sprites.

AnimatedSprite::AnimatedSprite( SDL_Surface *			sheet,
								AnimationGroup const *	animations,
								float 					x/* = 0*/,
								float 					y/* = 0*/ )
	:	Sprite( sheet, MakeRect( 0, 0, 0, 0 ), 0, 0, x, y ),
		m_pAnimations( animations ),
		m_currentAnimation( 0 ),
		m_currentFrame( 0 ),
		m_time( 0.0f ),
		m_frameTime( 0.0f ),
		m_direction( DIR_FORWARD )
{
	assert( animations != 0 );
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

AnimatedSprite::~AnimatedSprite()
{
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function starts a sprite's animation. The initial frame depends on the direction of play.
//!
//! @param	index			Which animation to play
//! @param	direction		Which direction to play the animation

void AnimatedSprite::PlayAnimation( int index, Direction direction/* = DIR_FORWARD*/ )
{
	assert( index >= 0 && index < int( m_pAnimations->animations.size() ) );

	Animation const &				animation	= m_pAnimations->animations[ index ];	// Convenience
	Animation::FrameList const &	frames		= animation.frames;						// Convenience

	m_currentAnimation	= index;
	m_direction			= direction;
	m_currentFrame		= ( direction == DIR_FORWARD ) ? 0 : int( frames.size() ) - 1;
	m_time				= 0.0f;
	m_frameTime			= 0.0f;

	// Set the image location and size according to the current frame

	Frame const &						frame	= frames[ m_currentFrame ];	// Convenience
	AnimationGroup::ImageList const &	images	= m_pAnimations->images;	// Convenience
	AnimationGroup::Image const &		image	= images[ frame.index ];	// Convenience

	m_rect		= image.rect;
	m_offsetX	= image.offsetX;
	m_offsetY	= image.offsetY;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function sets the animation time to an absolute value and updates the animation to reflect the new time.
//!
//! @param	time	new time
//!
//! @note If the mode is ping-pong, then the initial direction is assumed to be forward.

void AnimatedSprite::SetTime( float time )
{
	Animation const &	animation	= m_pAnimations->animations[ m_currentAnimation ];	// Convenience

	if ( animation.mode == Animation::MODE_PINGPONG )
	{
		m_direction = DIR_FORWARD;
	}

	PlayAnimation( m_currentAnimation, m_direction );
	AdvanceTime( time );
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function advances the animation by the specified amount of time
//!
//! @param	elapsed		elapsed time (must be >= 0)

void AnimatedSprite::AdvanceTime( float elapsed )
{
	assert( elapsed >= 0.0f );
	assert( m_currentAnimation >= 0 && m_currentAnimation < int( m_pAnimations->animations.size() ) );

	Animation const &				animation	= m_pAnimations->animations[ m_currentAnimation ];	// Convenience
	Animation::FrameList const &	frames		= animation.frames;									// Convenience

	if ( elapsed <= 0.0f )
	{
		return;
	}

	float e	= elapsed + m_frameTime;

	// Set the initial parameters based on the current direction

	int step;
	int first;
	int last;

	switch ( m_direction )
	{
	case DIR_FORWARD:
		step	= +1;
		first	= 0;
		last	= int( frames.size() ) - 1;
		break;
	case DIR_BACKWARD:
		step	= -1;
		first	= int( frames.size() ) - 1;
		last	= 0;
		break;
	}

	// Step through the animation until the correct frame is reached

	while ( e > 0.0f )
	{
		float currentFrameTime	= frames[ m_currentFrame ].time;

		if ( e >= currentFrameTime )
		{
			if ( m_currentFrame * step < last * step )
			{
				m_currentFrame += step;
			}
			else
			{
				if ( animation.mode == Animation::MODE_LOOP )
				{
					m_currentFrame = first;
				}
				else if ( animation.mode == Animation::MODE_PINGPONG )
				{
					m_direction = ( m_direction == DIR_FORWARD ) ? DIR_BACKWARD : DIR_FORWARD;
					step = -step;
					std::swap( first, last );
				}
			}
		}
		else
		{
			m_frameTime = e;
		}

		e -= currentFrameTime;
	}

	// Set the image location and size according to the current frame

	Frame const &						frame	= frames[ m_currentFrame ];		// Convenience
	AnimationGroup::ImageList const &	images	= m_pAnimations->images;		// Convenience
	AnimationGroup::Image const &		image	= images[ frame.index ];		// Convenience

	m_rect		= image.rect;
	m_offsetX	= image.offsetX;
	m_offsetY	= image.offsetY;

	// Save the time

	m_time += elapsed;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function sets the current animation to a specific frame (as opposed to a specific time). The running time
//! of the animation is then set to correspond to the specified frame. 
//!
//! @param	index	Desired frame
//!
//! @note If the mode is ping-pong, then the initial direction is assumed to be forward.

void AnimatedSprite::SetFrame( int index )
{
	Animation const &				animation	= m_pAnimations->animations[ m_currentAnimation ];	// Convenience
	Animation::FrameList const &	frames		= animation.frames;									// Convenience

	assert( index >= 0 && index < int( frames.size() ) - 1 );

	m_time = 0.0f;

	if ( animation.mode == Animation::MODE_PINGPONG )
	{
		m_direction = DIR_FORWARD;
	}

	switch ( m_direction )
	{
	case DIR_FORWARD:
		for ( int i = 0; i < index; ++i )
		{
			m_time += frames[i].time;
		}
		break;

	case DIR_BACKWARD:
		for ( int i = int( frames.size() ) - 1; i > index; ++i )
		{
			m_time += frames[i].time;
		}
		break;
	}

	m_currentFrame	= index;
	m_frameTime		= 0.0f;

	// Set the image location and size according to the current frame

	Frame const &						frame	= frames[ m_currentFrame ];		// Convenience
	AnimationGroup::ImageList const &	images	= m_pAnimations->images;		// Convenience
	AnimationGroup::Image const &		image	= images[ frame.index ];		// Convenience

	m_rect		= image.rect;
	m_offsetX	= image.offsetX;
	m_offsetY	= image.offsetY;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function will update the animation based on the amount of time that has elapsed since it was last called.
//!
//! @param	elapsedTime		Time elapsed since this function was last called (must be >= 0)

void AnimatedSprite::Service( float elapsedTime )
{
	assert( elapsedTime >= 0.0f );

	AdvanceTime( elapsedTime );
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! @param	images			List of images used in the animations
//! @param	animations		List of animations

AnimatedSprite::AnimationGroup::AnimationGroup( ImageList const & images, AnimationList const & animations )
	:	images( images ),
		animations( animations )
{
}

/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function creates a sprite described by the specified file. Ownership of the sprite is passed to the
//! caller. The sprite must be deallocated using delete.
//!
//! @param	filename	name of the file containing the sprite description
//!
//! @return	The new sprite.

Sprite * SpriteFactory::LoadSprite( char const * filename )
{
	return 0;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function creates an animated sprite described by the specified file. Ownership of the sprite is passed to
//! the caller. The sprite must be deallocated using delete.
//!
//! @param	filename	name of the file containing the sprite description
//!
//! @return	The new animated sprite.

AnimatedSprite * SpriteFactory::LoadAnimatedSprite( char const * filename )
{
	return 0;
}



} // namespace Sdlx


