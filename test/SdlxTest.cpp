// Tests for the helpers in Sdlx.h and for Sprite::Draw. These run headless: they use in-memory surfaces and the
// SDL event queue, and never create a window.

#include <Sdlx/Sdlx.h>
#include <Sdlx/Sprite.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

namespace
{

struct Pixel
{
	Uint8 r, g, b;
	bool operator==( Pixel const & o ) const { return r == o.r && g == o.g && b == o.b; }
};

std::ostream & operator<<( std::ostream & os, Pixel const & p )
{
	return os << "(" << int( p.r ) << "," << int( p.g ) << "," << int( p.b ) << ")";
}

Pixel const BLACK   = { 0, 0, 0 };
Pixel const RED     = { 255, 0, 0 };
Pixel const GREEN   = { 0, 255, 0 };
Pixel const BLUE    = { 0, 0, 255 };
Pixel const MAGENTA = { 255, 0, 255 };

SDL_Surface * NewSurface( int w, int h, Pixel fill )
{
	SDL_Surface * s = SDL_CreateSurface( w, h, SDL_PIXELFORMAT_RGBA32 );
	if ( s != nullptr )
	{
		SDL_FillSurfaceRect( s, nullptr, SDL_MapSurfaceRGB( s, fill.r, fill.g, fill.b ) );
	}
	return s;
}

void Fill( SDL_Surface * s, SDL_Rect rect, Pixel p )
{
	SDL_FillSurfaceRect( s, &rect, SDL_MapSurfaceRGB( s, p.r, p.g, p.b ) );
}

Pixel At( SDL_Surface * s, int x, int y )
{
	Pixel p = { 0, 0, 0 };
	Uint8 a;
	EXPECT_TRUE( SDL_ReadSurfacePixel( s, x, y, &p.r, &p.g, &p.b, &a ) ) << SDL_GetError();
	return p;
}

// Owns a surface for the duration of a test
struct SurfacePtr
{
	explicit SurfacePtr( SDL_Surface * s = nullptr ) : surface( s ) {}
	~SurfacePtr() { SDL_DestroySurface( surface ); }
	SurfacePtr( SurfacePtr const & ) = delete;
	SurfacePtr & operator=( SurfacePtr const & ) = delete;
	SDL_Surface * get() const { return surface; }
	SDL_Surface * surface;
};

// A temporary file path that is removed when the object goes out of scope
struct TempFile
{
	explicit TempFile( std::string const & name )
		: path( ( std::filesystem::temp_directory_path() / name ).string() )
	{
	}
	~TempFile() { std::remove( path.c_str() ); }
	std::string path;
};

} // anonymous namespace

/********************************************************************************************************************/
/* Helpers                                                                                                          */
/********************************************************************************************************************/

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#elif defined( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

TEST( HelpersTest, MakeRectSetsFields )
{
	SDL_Rect r = Sdlx::MakeRect( 1, 2, 3, 4 );

	EXPECT_EQ( r.x, 1 );
	EXPECT_EQ( r.y, 2 );
	EXPECT_EQ( r.w, 3 );
	EXPECT_EQ( r.h, 4 );
}

TEST( HelpersTest, MakeColorSetsFieldsAndIsOpaque )
{
	SDL_Color c = Sdlx::MakeColor( 10, 20, 30 );

	EXPECT_EQ( c.r, 10 );
	EXPECT_EQ( c.g, 20 );
	EXPECT_EQ( c.b, 30 );
	EXPECT_EQ( c.a, 255 );
}

#if defined( _MSC_VER )
#pragma warning( pop )
#elif defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

/********************************************************************************************************************/
/* Image loading                                                                                                    */
/********************************************************************************************************************/

TEST( LoadImageTest, LoadsBmp )
{
	SurfacePtr source( NewSurface( 4, 2, RED ) );
	Fill( source.get(), SDL_Rect{ 2, 0, 2, 2 }, GREEN );
	TempFile file( "sdlx_test_load.bmp" );
	ASSERT_TRUE( SDL_SaveBMP( source.get(), file.path.c_str() ) ) << SDL_GetError();

	SurfacePtr image( Sdlx::LoadImage( file.path.c_str() ) );

	ASSERT_NE( image.get(), nullptr ) << SDL_GetError();
	EXPECT_EQ( image.get()->w, 4 );
	EXPECT_EQ( image.get()->h, 2 );
	EXPECT_EQ( At( image.get(), 0, 0 ), RED );
	EXPECT_EQ( At( image.get(), 3, 1 ), GREEN );
}

TEST( LoadImageTest, LoadsPng )
{
	SurfacePtr source( NewSurface( 3, 3, BLUE ) );
	TempFile file( "sdlx_test_load.png" );
	ASSERT_TRUE( SDL_SavePNG( source.get(), file.path.c_str() ) ) << SDL_GetError();

	SurfacePtr image( Sdlx::LoadImage( file.path.c_str() ) );

	ASSERT_NE( image.get(), nullptr ) << SDL_GetError();
	EXPECT_EQ( image.get()->w, 3 );
	EXPECT_EQ( image.get()->h, 3 );
	EXPECT_EQ( At( image.get(), 1, 1 ), BLUE );
}

TEST( LoadImageTest, MissingFileReturnsNull )
{
	EXPECT_EQ( Sdlx::LoadImage( "this/file/does/not/exist.png" ), nullptr );
}

TEST( LoadColorKeyedImageTest, SetsColorKey )
{
	SurfacePtr source( NewSurface( 2, 1, MAGENTA ) );
	TempFile file( "sdlx_test_key.bmp" );
	ASSERT_TRUE( SDL_SaveBMP( source.get(), file.path.c_str() ) ) << SDL_GetError();

	SurfacePtr image( Sdlx::LoadColorKeyedImage( file.path.c_str(), SDL_Color{ 255, 0, 255, 255 } ) );

	ASSERT_NE( image.get(), nullptr ) << SDL_GetError();
	EXPECT_TRUE( SDL_SurfaceHasColorKey( image.get() ) );

	Uint32 key = 0;
	ASSERT_TRUE( SDL_GetSurfaceColorKey( image.get(), &key ) );
	EXPECT_EQ( key, SDL_MapSurfaceRGB( image.get(), 255, 0, 255 ) );
}

TEST( LoadColorKeyedImageTest, KeyedPixelsAreNotBlitted )
{
	SurfacePtr source( NewSurface( 2, 1, MAGENTA ) );
	Fill( source.get(), SDL_Rect{ 1, 0, 1, 1 }, RED );
	TempFile file( "sdlx_test_keyblit.bmp" );
	ASSERT_TRUE( SDL_SaveBMP( source.get(), file.path.c_str() ) ) << SDL_GetError();

	SurfacePtr image( Sdlx::LoadColorKeyedImage( file.path.c_str(), SDL_Color{ 255, 0, 255, 255 } ) );
	ASSERT_NE( image.get(), nullptr ) << SDL_GetError();

	SurfacePtr dst( NewSurface( 2, 1, BLUE ) );
	ASSERT_TRUE( SDL_BlitSurface( image.get(), nullptr, dst.get(), nullptr ) ) << SDL_GetError();

	EXPECT_EQ( At( dst.get(), 0, 0 ), BLUE );		// the key color is transparent
	EXPECT_EQ( At( dst.get(), 1, 0 ), RED );
}

TEST( LoadColorKeyedImageTest, MissingFileReturnsNull )
{
	EXPECT_EQ( Sdlx::LoadColorKeyedImage( "this/file/does/not/exist.png", SDL_Color{ 0, 0, 0, 255 } ), nullptr );
}

/********************************************************************************************************************/
/* Sprite::Draw                                                                                                     */
/********************************************************************************************************************/

namespace
{

// An 8x8 sheet: the left half is red and the right half is green
class SpriteDrawTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		sheet.reset( new SurfacePtr( NewSurface( 8, 8, RED ) ) );
		Fill( sheet->get(), SDL_Rect{ 4, 0, 4, 8 }, GREEN );
		dst.reset( new SurfacePtr( NewSurface( 32, 32, BLACK ) ) );
	}

	std::unique_ptr< SurfacePtr > sheet;
	std::unique_ptr< SurfacePtr > dst;
};

} // anonymous namespace

TEST_F( SpriteDrawTest, DrawsSubRegionAtPositionMinusOffset )
{
	Sdlx::Sprite sprite( sheet->get(), SDL_Rect{ 4, 0, 4, 8 }, 1, 1, 10.0f, 10.0f );
	sprite.Draw( dst->get() );

	// Drawn at (9, 9), size 4x8
	EXPECT_EQ( At( dst->get(), 9, 9 ), GREEN );
	EXPECT_EQ( At( dst->get(), 12, 16 ), GREEN );
	EXPECT_EQ( At( dst->get(), 8, 9 ), BLACK );
	EXPECT_EQ( At( dst->get(), 13, 9 ), BLACK );
	EXPECT_EQ( At( dst->get(), 9, 8 ), BLACK );
	EXPECT_EQ( At( dst->get(), 9, 17 ), BLACK );
}

TEST_F( SpriteDrawTest, EmptyRectDrawsWholeSheet )
{
	Sdlx::Sprite sprite( sheet->get(), SDL_Rect{ 0, 0, 0, 0 }, 0, 0, 2.0f, 3.0f );
	sprite.Draw( dst->get() );

	EXPECT_EQ( At( dst->get(), 2, 3 ), RED );
	EXPECT_EQ( At( dst->get(), 5, 3 ), RED );
	EXPECT_EQ( At( dst->get(), 6, 3 ), GREEN );
	EXPECT_EQ( At( dst->get(), 9, 10 ), GREEN );
	EXPECT_EQ( At( dst->get(), 10, 10 ), BLACK );
}

TEST_F( SpriteDrawTest, PositionIsRoundedToNearestPixel )
{
	Sdlx::Sprite down( sheet->get(), SDL_Rect{ 0, 0, 0, 0 }, 0, 0, 10.4f, 10.4f );
	down.Draw( dst->get() );
	EXPECT_EQ( At( dst->get(), 10, 10 ), RED );
	EXPECT_EQ( At( dst->get(), 9, 10 ), BLACK );

	SurfacePtr other( NewSurface( 32, 32, BLACK ) );
	Sdlx::Sprite up( sheet->get(), SDL_Rect{ 0, 0, 0, 0 }, 0, 0, 10.6f, 10.6f );
	up.Draw( other.get() );
	EXPECT_EQ( At( other.get(), 11, 11 ), RED );
	EXPECT_EQ( At( other.get(), 10, 10 ), BLACK );
}

TEST_F( SpriteDrawTest, AnimatedSpriteDrawsCurrentFrame )
{
	Sdlx::AnimatedSprite::AnimationGroup group;
	group.images.push_back( { SDL_Rect{ 0, 0, 4, 8 }, 0, 0 } );		// red
	group.images.push_back( { SDL_Rect{ 4, 0, 4, 8 }, 0, 0 } );		// green
	Sdlx::AnimatedSprite::Animation animation;
	animation.mode   = Sdlx::AnimatedSprite::Animation::MODE_LOOP;
	animation.frames = { { 0, 1.0f }, { 1, 1.0f } };
	group.animations.push_back( animation );

	Sdlx::AnimatedSprite sprite( sheet->get(), &group, 4.0f, 4.0f );
	sprite.PlayAnimation( 0 );
	sprite.Draw( dst->get() );
	EXPECT_EQ( At( dst->get(), 4, 4 ), RED );

	sprite.AdvanceTime( 1.0f );
	sprite.Draw( dst->get() );
	EXPECT_EQ( At( dst->get(), 4, 4 ), GREEN );
}

/********************************************************************************************************************/
/* EventLoop                                                                                                        */
/********************************************************************************************************************/

namespace
{

int  g_events;
int  g_idleCalls;
int  g_stopAfter;		// handler returns false after this many events (0 = until quit)

void PushEvent( Uint32 type )
{
	SDL_Event e;
	SDL_zero( e );
	e.type = type;
	ASSERT_TRUE( SDL_PushEvent( &e ) ) << SDL_GetError();
}

bool CountingHandler( SDL_Event const & event )
{
	++g_events;
	if ( g_stopAfter > 0 && g_events >= g_stopAfter )
	{
		return false;
	}
	return event.type != SDL_EVENT_QUIT;
}

bool IdleThenQuit()
{
	++g_idleCalls;
	if ( g_idleCalls == 3 )
	{
		PushEvent( SDL_EVENT_QUIT );
	}
	return true;
}

bool IdleOnceThenQuit()
{
	++g_idleCalls;
	PushEvent( SDL_EVENT_QUIT );
	return false;		// do not call again until after the next event
}

class EventLoopTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite() { ASSERT_TRUE( SDL_Init( SDL_INIT_EVENTS ) ) << SDL_GetError(); }
	static void TearDownTestSuite() { SDL_Quit(); }

	void SetUp() override
	{
		SDL_FlushEvents( SDL_EVENT_FIRST, SDL_EVENT_LAST );
		g_events    = 0;
		g_idleCalls = 0;
		g_stopAfter = 0;
	}
};

} // anonymous namespace

TEST_F( EventLoopTest, DefaultHandlerReturnsOnQuit )
{
	PushEvent( SDL_EVENT_QUIT );

	Sdlx::EventLoop();		// would hang if the quit event were not recognized
	SUCCEED();
}

TEST_F( EventLoopTest, HandlerSeesEveryEventUntilQuit )
{
	PushEvent( SDL_EVENT_USER );
	PushEvent( SDL_EVENT_USER );
	PushEvent( SDL_EVENT_USER );
	PushEvent( SDL_EVENT_QUIT );

	Sdlx::EventLoop( CountingHandler );

	EXPECT_EQ( g_events, 4 );
}

TEST_F( EventLoopTest, HandlerReturningFalseStopsTheLoop )
{
	g_stopAfter = 2;
	PushEvent( SDL_EVENT_USER );
	PushEvent( SDL_EVENT_USER );
	PushEvent( SDL_EVENT_USER );

	Sdlx::EventLoop( CountingHandler );

	EXPECT_EQ( g_events, 2 );
}

TEST_F( EventLoopTest, IdleCallbackRunsWhenThereAreNoEvents )
{
	Sdlx::EventLoop( nullptr, IdleThenQuit );

	EXPECT_EQ( g_idleCalls, 3 );
}

TEST_F( EventLoopTest, IdleCallbackReturningFalseIsNotCalledAgainUntilNextEvent )
{
	Sdlx::EventLoop( nullptr, IdleOnceThenQuit );

	EXPECT_EQ( g_idleCalls, 1 );
}
