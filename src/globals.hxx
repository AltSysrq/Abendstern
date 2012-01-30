/**
 * @file
 * @author Jason Lingle
 *
 * @brief Declarations of common global variables.
 */

#ifndef GLOBALS_HXX_
#define GLOBALS_HXX_

//Asio for some reason depends on boost regex, which at one point uses a value
//named "pi"...
//Include it before definition
#include <asio.hpp>

//Include time now so we don't collide below
#include <ctime>
#ifdef DEBUG
#include <cmath> //needed by fmod replacement
#endif /* DEBUG */
//Profiling only works on UNIX
#ifdef PROFILE
#include <sys/time.h>
#endif

#define vclock_t Uint32
#define VCLOCKS_PER_SEC 1000

#include <utility>
//For Uint32
#include <GL/gl.h>
#include <SDL.h>

#ifdef WIN32
#define PLATFORM "WINDOWS"
#else
/** Defines the name of the current platform, sometimes used by scripts. */
#define PLATFORM "UNIX"
#endif

/* Support for graphics profiling */
#ifndef PROFILE
  #define BEGINGP(name) {
  #define ENDGP }
#else
  #include <map>
  #define BEGINGP(name) { \
    timeval gp_begin; \
    gettimeofday(&gp_begin,NULL); \
    const char*const gp_name="gp__"name;
  #define ENDGP \
    timeval gp_end; \
    gettimeofday(&gp_end,NULL); \
    useconds_t gp_time=gp_end.tv_usec-gp_begin.tv_usec; \
    gp_time %= 1000000; \
    float gp_secs=gp_time/1000000.0f; \
    gp_profile[gp_name]+=gp_secs; \
    gp_currFPSProfile[gp_name]+=gp_secs; \
  }

  //We map against the pointer itself (which is fast),
  //since the key will only be used in exactly one
  //place ever (from within BEGINGP)
  extern std::map<const char*, float> gp_profile, gp_currFPSProfile;
#endif

#include "secondary/confreg.hxx"

class GameState;

/** The currently running GameState.
 * Specifically, this is the one that will receive input events and have
 * its GameState::update and GameState::draw functions called. Nested
 * GameStates will therefore not be stored here, and no GameState may
 * count on state == this being true.
 */
extern GameState* state;

extern unsigned
  /** The physical width, in pixels, of the window. */  screenW,
  /** The physical height, in pixels, of the window. */ screenH;

/** The greatest on-screen value for height. */
extern float vheight;

extern bool
  /** Determines whether unnecessary alpha blending should be used.
   * If true, all cases that could use alpha blending should.
   * If false, alpha blending should only be used in cases considered
   * "very important".
   */
  generalAlphaBlending,
  /** Determines whether alpha blending is enabled at the GL level.
   * If true, alpha blending may be used. If false, all rendering
   * will be opaque.
   */
  alphaBlendingEnabled,
  /** Determines whether GL_NEAREST or GL_LINEAR should be used.
   * If true, OpenGL should be set to use GL_LINEAR for texture scaling
   * when possible.
   * If false, it should use GL_NEAREST when possible.
   */
  smoothScaling,
  /** Determines whether purely-graphical effects should occur.
   * These include, for example, the plasma fire and cell fragment effects.
   */
  highQuality,
  /** Determines whether points and lines will be antialiased. */
  antialiasing;

/** Set to true if no graphics operations will be performed.
  * If this is true, GL operations are invalid, and there is no
  * screen device and will be no input events. Additionally,
  * the GameState::draw function will never be called.
  */
extern bool headless;

//Set by the camera. Anything between these might be on-screen.
//Prior to 2010.01.07, the centre of the screen was assumed to
//be ((camerX1+cameraX2)/2, (cameraY1+cameraY2)/2); however, this
//is no longer the case, and code must use cameraCX and cameraCY
//for that.
extern float
  /** Indicates the lowest possible X coordinate that may be on-screen. */
  cameraX1,
  /** Indicates the greatest possible X coordinate that may be on-screen. */
  cameraX2,
  /** Indicates the lowest possible Y coordinate that may be on-screen. */
  cameraY1,
  /** Indicates the greatest possible Y coordinate that may be on-screen. */
  cameraY2;
extern float
  /** Indicates the X coordinate that is the centre of the screen. */
  cameraCX,
  /** Indicates the Y coordinate that is the centre of the screen. */
  cameraCY;
/** Indicates the current camera zoom.
 * This is actually a scaling factor; lower values result in smaller
 * graphics (but more on screen), "zoomed out", while higher values
 * are "zoomed in".
 */
extern float cameraZoom;

//These are maintained by the current GameState, if it
//needs them.
extern int cursorX, cursorY, oldCursorX, oldCursorY;

/** The actual coördinates of the screen corners.
 * No order is defined, other than that the four points
 * will form a rectangle when drawn in that order.
 */
extern std::pair<float,float> screenCorners[4];

/** Indicates the total elapsed time in the current physical frame. */
extern float currentFrameTime,
  /** Indicase the time remaining in the current physical frame.
   * This includes the time of the current virtual frame.
   * If operating at decorative time, this value is undefined.
   */
  currentFrameTimeLeft;
/** Set to true if the current virtual frame is the last in the current physical frame. */
extern bool currentVFrameLast;

/** The current frame rate, in frames/sec.
 * This is recalculated periodically by the kernel.
 */
extern unsigned frameRate;

/** This is the multiplier originally defined in ExplosionPool.hxx. It can be used
 * for other graphical adjustments as well. It is const to the rest of the world
 * other than ExplosionPool.
 */
extern float sparkCountMultiplier;

/** Everybody's favourite constant. */
#define pi 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679f

/** Stores the number of milliseconds that have elapsed since the game loop started.
 * Functions should not be adversely affected by this warping around,
 * even though that won't happen until after at least 49 days, even
 * on systems where long is only 32 bits.
 */
extern unsigned long gameClock;

/** Indicates whether fast-forward mode is enabled.
 *
 * In fast-forward mode, any frame whose elapsed time is less than 10 milliseconds
 * will be treated as if it were exactly 10 milliseconds.
 *
 * As this is intended for headless, non-network-play modes, it also fixes the maximum
 * time for a frame to 10 ms (as it is useless in any other case).
 */
extern bool fastForward;

/** The central configuration root.
 * This is the singleton instance of ConfReg.
 */
extern ConfReg conf;

extern bool debug_freeze;

#ifdef DEBUG
/** Same as fmod(), but doesn't raise an FPE when the numerator
 * is zero (which GNU libm does, for some reason).
 */
static inline float dbfmod(float x, float y) {
  return x == 0? 0 : std::fmod(x,y);
}
/** @see dbfmod() */
#define fmod(x,y) dbfmod(x,y)
#endif /* DEBUG */

/** Convenience macro to find the length of an array. */
#define lenof(x) (sizeof(x)/sizeof(x[0]))

/** Evaluates to true if the given coordinate is "close enough" for special effects.
 * It is used as a general test to see whether the player will even see an effect;
 * if not, there is no point in creating it.
 */
#define EXPCLOSE(x,y) (x > cameraX1-2 && \
                       x < cameraX2+2 && \
                       y > cameraY1-2 && \
                       y < cameraY2+2)

/** Evaluates to the appropriate GL texture scaling method.
 * Based on the smoothScaling value.
 */
#define IMG_SCALE (smoothScaling? GL_LINEAR : GL_NEAREST)

/** Defines the filename of the primary configuration file. */
#define CONFIG_FILE "abendstern.rc"
/** Defines the filename of the default configuration file. */
#define DEFAULT_CONFIG_FILE "abendstern.default.rc"
/** Defines room temperature in Kelvin. */
#define ROOM_TEMPERATURE 293.15f

#endif /*GLOBALS_HXX_*/
