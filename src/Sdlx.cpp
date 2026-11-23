/** @file *//********************************************************************************************************

                                                       Sdlx.cpp

						                    Copyright 2006, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/Sdlx/Sdlx.cpp#1 $

	$NoKeywords: $

 ********************************************************************************************************************/

#include "Sdlx.h"

#include <SDL3_image/SDL_image.h>

namespace
{

	bool DefaultEventLoopEventHandler( SDL_Event const & event )
	{
		return ( event.type != SDL_EVENT_QUIT );
	}


} // anonymous namespace


namespace Sdlx
{


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function loads an image file. All image formats supported by SDL3_image can be loaded. Currently, they are:
//!		- BMP
//!		- JPEG
//!		- PNG
//!		- GIF
//!		- TGA
//!		- and more
//!
//! SDL3 has no "display format", so the image is returned in the format in which it was loaded.
//!
//! @param	filename	name of the file to load
//!
//! @return		pointer to the loaded surface (free with SDL_DestroySurface), or 0 if error

SDL_Surface * LoadImage( char const * filename )
{
	return IMG_Load( filename );
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function loads an image file. In addition, a color key is specified.
//!
//! All image formats supported by SDL3_image can be loaded. Currently, they are:
//!		- BMP
//!		- JPEG
//!		- PNG
//!		- GIF
//!		- TGA
//!		- and more
//!
//! @param	filename	name of the file to load
//! @param	key			color that is treated as transparent
//!
//! @return		pointer to the loaded surface (free with SDL_DestroySurface), or 0 if error

SDL_Surface * LoadColorKeyedImage( char const * filename, SDL_Color key ) 
{
	SDL_Surface *	image	= LoadImage( filename );

	if ( image != 0 )
    {
		Uint32 colorkey = SDL_MapSurfaceRGB( image, key.r, key.g, key.b );
		SDL_SetSurfaceColorKey( image, true, colorkey );

		// Note: RLE acceleration (SDL_SetSurfaceRLE) is deliberately not enabled. With SDL3, an RLE-encoded surface
		// that has an alpha channel ignores the color key when blitted.
    }

	return image;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function handles SDL events until a SDL_EVENT_QUIT event is received. If an idle processing callback is
//! specified, it is called whenever there are no events to process. If the callback returns @c false, then
//! it will not be called again until after the next event occurs.
//!
//!	@param	pEH			Event Handler callback
//!	@param	pIdle		Idle processing callback

void EventLoop( EventLoopEventHandler pEH/* = 0*/, EventLoopIdleCallback pIdle/* = 0*/ )
{
	bool	done		= false;
	bool	skipIdle	= false;

	if ( pEH == 0 )
	{
		pEH = DefaultEventLoopEventHandler;
	}

	while ( !done )
	{
		SDL_Event	event;
		bool		haveEvent;

		// Get an event. If there is no idle function to call (or it no longer needs to be called), then block until
		// an event arrives, otherwise just check for one.

		if ( skipIdle || pIdle == 0 )
		{
			haveEvent = SDL_WaitEvent( &event );

			if ( !haveEvent )
			{
				SDL_Delay( 1 );		// Don't spin if waiting failed
			}
		}
		else
		{
			haveEvent = SDL_PollEvent( &event );
		}

		if ( haveEvent )
		{
			// Call the event handler. If it returns false, then we should exit.
			done = !(*pEH)( event );

			skipIdle = false;		// Now that we have processed a message, start calling the callback function again.
        }
		else if ( skipIdle || pIdle == 0 )
		{
			// Nothing to do
		}
		else
		{
			// Call the idle function. If it returns false, it no longer needs to be called -- set 'skipIdle' so it
			// won't be called again until after another message is processed.

			skipIdle = !(*pIdle)();
		}
    }
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function is basically a convenience. SDL structs lack constructors, so this acts as a constructor for
//! @c SDL_Color. The alpha component is set to opaque.
//!
//! @param	red		red component. The valid range is 0 - 255.
//! @param	green	green component. The valid range is 0 - 255.
//! @param	blue	blue component. The valid range is 0 - 255.
//!
//! @deprecated	Use brace initialization instead: <tt>SDL_Color{ r, g, b, 255 }</tt>.

SDL_Color MakeColor( int red, int green, int blue )
{
	SDL_Color	color	= { Uint8( red ), Uint8( green ), Uint8( blue ), 255 };

	return color;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function is basically a convenience. SDL structs lack constructors, so this acts as a constructor for
//! @c SDL_Rect.
//!
//! @param	x	x
//! @param	y	y
//! @param	w	width
//! @param	h	height
//!
//! @deprecated	Use brace initialization instead: <tt>SDL_Rect{ x, y, w, h }</tt>.

SDL_Rect MakeRect( int x, int y, int w, int h )
{
	SDL_Rect	rect	= { x, y, w, h };

	return rect;
}


} // namespace Sdlx
