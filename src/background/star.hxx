/**
 * @file
 * @author Jason Lingle
 *
 * @brief Defines the Star class, used internally by StarField.
 */
#ifndef STAR_HXX_
#define STAR_HXX_

#include <GL/gl.h>
#include "src/opto_flags.hxx"

/** Used internally by StarField and Star. */
extern GLuint starTextures[9];

/** Class used internally by StarField.
 *
 * Stars are used in the background to give the player a sense of motion,
 * especially when he is alone in deep space. Most stars are whitish,
 * but some will be between blue and red.
 * You may notice that, despite the fact that we use OpenGL, this class
 * completely cheats at 3D. Each star has a "distance" between 0.1 and 0.9,
 * where lower numbers are further away. Each frame, a star is updated with
 * the delta of how far the player has moved. A star moves in the opposite
 * direction, multiplied by its distance.
 * In reality, paralax is non-linear; however, the number of stars visible
 * also increases non-linearly with increased distance, rendering the paralax
 * distribution conveniently linear. I think.
 *
 * When drawing, the stars are shorn according to the velocity of the player,
 * multiplied (of course) by paralax.
 *
 * Stars are to be forgotten when x or y is beyond the range of -1..2.
 * See also star_field.hxx
 */
class Star {
  private:
  float distance, x, y;
  float colourR, colourG, colourB;
  unsigned tex;

  public:

  /** Constructs a new Star at the given coordinates.
   * distance is determined by selecting a random
   * number. Either x or y should have a value of -1 or 2.
   */
  void reset(float x, float y, float dist) noth;
  /** Constructs a new Star at the given coordinates.
   * distance is determined by selecting a random
   * number. Either x or y should have a value of -1 or 2.
   */
  inline void reset(float x, float y) noth {
    reset(x, y, distance);
  }

  /** Returns the X coordinate */
  inline float getX() const noth { return x; }
  /** Returns the Y coordinate */
  inline float getY() const noth { return y; }
  /** Returns the Z coordinate */
  inline float getZ() const noth { return distance; }

  /** Moves the star according to the specified deltata */
  void move(float deltaX, float deltaY) noth;
  /**
   * Draws the star, using one of the 9 display lists
   * in Star::sizes. The list selected is at (int)((distance-0.1)*10).
   */
  void draw(float shear, float angle, float cosAngle, float sinAngle) noth;

  /** Returns true if the star should cease to exist. */
  bool shouldDelete() noth;
};

#endif /*STAR_HXX_*/
