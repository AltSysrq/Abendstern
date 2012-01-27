/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the Planet Background class.
 */

#ifndef PLANET_HXX_
#define PLANET_HXX_

#include <GL/gl.h>
#include "src/core/aobject.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/camera/effects_handler.hxx"
#include "src/opto_flags.hxx"
#include "background.hxx"

/** A Planet is an alternate background to the StarField. It is
 * formed by a composite of a day and night image, which
 * blend into each other. The planet constantly rotates (a
 * sliding of the day/night boundary), and the planet appears
 * to rotate even faster (orbit). There is also a parallax such
 * that the very top of the planet image is visible at the very
 * top of the GameField, and the very bottom is visible from
 * the very bottom. The same level of parallax is applied horiz-
 * ontally, so that the "true" position of the planet image is
 * visible at the very centre of the GameField.
 *
 * Since the images are quite large, they are decomposed into
 * 64x256 px textures (the width is low to give a smooth day/
 * night blend).
 */
class Planet : public Background {
  private:
  GLuint* textures;
  GLuint vao, vbo;

  int numTexturesWide, numTexturesHigh;
  GameObject* reference;
  GameField* field;
  float parallax;
  /* tileSize is the height of each tile.
   * The width of a tile can be determined
   * by tileSize*TEXW/TEXH.
   */
  float tileSize;
  /* Twilight defines the percentange around
   * the circle that is gradiented between
   * day and night.
   */
  float twilight;

  float rev, orbit;
  float vrev, vorbit;

  //Explosions in the night sky
  float glareR, glareG, glareB;

  public:
  /** Constructs a new Planet with the given parms.
   *
   * @param ref Initial GameObject to use as reference; may be NULL
   * @param field The field this is a background too
   * @param day Filename for the daytime image
   * @param night Filename for the nighttime image
   * @param revolutionTime Time in milliseconds for an entire day/night cycle
   * @param orbitTime Time in milliseconds for the planet to visually revolve (relative to static objects)
   * @param height How high, in screens, the background should stretch
   * @param twilight The width of the Twilight Zone (ie, the area where day and night are blended)
   */
  Planet(GameObject* ref, GameField* field,
         const char* day, const char* night, float revolutionTime, float orbitTime,
         float height, float twilight);
  virtual ~Planet();

  virtual void draw() noth;
  virtual void update(float) noth;
  virtual void updateReference(GameObject* ref, bool reset) noth;
  virtual void explode(Explosion*) noth;
};
#endif /*PLANET_HXX_*/
