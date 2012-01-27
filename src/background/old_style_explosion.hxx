/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the OldStyleExplosion class.
 */

#ifndef OLD_STYLE_EXPLOSION_HXX_
#define OLD_STYLE_EXPLOSION_HXX_

#include <GL/gl.h>

#include "src/sim/game_object.hxx"

class Explosion;

/** The OldStyleExplosion implements large spark explosions the way
 * the original Abendstern did (except with vertex arrays instead
 * of display lists).
 *
 * These are decorative, non-colideable GameObjects which have a
 * VAO/VBO pair containing a number of points.
 */
class OldStyleExplosion: public GameObject {
  const GLuint vao, vbo;
  const float r, g, b;
  const float sizeAt1Sec, maxLife;
  float currLife;
  unsigned numPoints;

  public:
  /** Constructs a new OldStyleExplosion with the given
   * Explosion as a reference.
   *
   * @param ex The Explosion to duplicate.
   */
  OldStyleExplosion(const Explosion* ex);
  virtual ~OldStyleExplosion();

  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual float getRadius() const noth;
};

#endif /* OLD_STYLE_EXPLOSION_HXX_ */
