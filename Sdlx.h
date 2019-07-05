/** @file *//********************************************************************************************************

                                                        Sdlx.h

						                    Copyright 2006, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/Sdlx/Sdlx.h#1 $

	$NoKeywords: $

 ********************************************************************************************************************/

#pragma once


#include <SDL.h>

namespace Sdlx
{
	//! Loads an image file.
	SDL_Surface * LoadImage( char const * filename );

	//! Loads an image file and applies a color key
	SDL_Surface * LoadColorKeyedImage( char const * filename, SDL_Color key ) ;

	//! Idle processing callback.
	typedef bool (*EventLoopIdleCallback)();

	//! Event loop event handler
	typedef bool (*EventLoopEventHandler)( SDL_Event const & event );

	//! Handle events until a SDL_QUIT event is received.
	void EventLoop( EventLoopEventHandler pEH = 0, EventLoopIdleCallback pCB = 0 );

	//! Returns a color with the specified values
	SDL_Color MakeColor( int red, int green, int blue );

	//! Returns a rect with the specified values
	SDL_Rect MakeRect( int x, int y, int w, int h );

} // namespace Sdlx

