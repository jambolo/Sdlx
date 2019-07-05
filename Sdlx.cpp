/** @file *//********************************************************************************************************

                                                       Sdlx.cpp

						                    Copyright 2006, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/Sdlx/Sdlx.cpp#1 $

	$NoKeywords: $

 ********************************************************************************************************************/

#include "PrecompiledHeaders.h"

#include "Sdlx.h"

namespace
{

	bool DefaultEventLoopEventHandler( SDL_Event const & event )
	{
		return ( event.type != SDL_QUIT );
	}


} // anonymous namespace


namespace Sdlx
{


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function loads an image file and converts it to the format of the display.
//! All image formats supported by SDL_image can be loaded. Currently, they are:
//!		- BMP
//!		- JPEG
//!		- PNG
//!		- GIF
//!		- TGA
//!		- and more
//!
//! @param	filename	name of the file to load
//!
//! @return		pointer to the loaded file, or 0 if error

SDL_Surface * LoadImage( char const * filename ) 
{
	SDL_Surface *	loadedImage		= 0;
	SDL_Surface *	image			= 0;

	loadedImage = IMG_Load( filename );

	if ( loadedImage != 0 )
	{
		image = SDL_DisplayFormat( loadedImage );	// Create an optimized image from the loaded image
		SDL_FreeSurface( loadedImage );				// Free the old image
	}

	return image;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function loads an image file and converts it to the format of the display. In addition, a color key is
//! specified.
//!
//! All image formats supported by SDL_image can be loaded. Currently, they are:
//!		- BMP
//!		- JPEG
//!		- PNG
//!		- GIF
//!		- TGA
//!		- and more
//!
//! @param	filename	name of the file to load
//!
//! @return		pointer to the loaded file, or 0 if error

SDL_Surface * LoadColorKeyedImage( char const * filename, SDL_Color key ) 
{
	SDL_Surface *	image	= LoadImage( filename );

	if ( image != 0 )
    {
		Uint32 colorkey = SDL_MapRGB( image->format, key.r, key.g, key.b );
		SDL_SetColorKey( image, SDL_RLEACCEL | SDL_SRCCOLORKEY, colorkey );
    }

	return image;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! This function handles SDL events until a SDL_QUIT event is received. If an idle processing callback is
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

		// Get an event. If there is none to process, then if we are skipping the idle function then 
		// just sleep, otherwise call the idle function.

		if ( SDL_PollEvent( &event ) != 0 )
		{
			// Call the event handler. If it returns false, then we should exit.
			done = !(*pEH)( event );

			skipIdle = false;		// Now that we have processed a message, start calling the callback function again.
        }
		else if ( skipIdle || pIdle == 0 )
		{
			SDL_Delay( 0 );
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
//! @c SDL_Color.
//!
//! @param	red		red component. The valid range is 0 - 255.
//! @param	green	green component. The valid range is 0 - 255.
//! @param	blue	blue component. The valid range is 0 - 255.

SDL_Color MakeColor( int red, int green, int blue )
{
	SDL_Color	color	= { red, green, blue };

	return color;
}

int foo( int a, int b, int c )
{
	SDL_Color	color	= { a, b, c };

	return 0;
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

SDL_Rect MakeRect( int x, int y, int w, int h )
{
	SDL_Rect	rect	= { x, y, w, h };

	return rect;
}


} // namespace Sdlx
