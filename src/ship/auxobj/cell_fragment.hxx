/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CellFragment class
 */

#ifndef CELL_FRAGMENT_HXX_
#define CELL_FRAGMENT_HXX_

#include <GL/gl.h>
#include <vector>

#include "src/sim/game_object.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/graphics/cmn_shaders.hxx"

class Blast;

/** CellFragments are high-quality visual effects spawned when a Cell
 * is destroyed. They are "collided" with the Blast that destroyed
 * the parent cell, but are otherwise non-collideable. When a set
 * is created, a texture used to be created from the cell that
 * was used for the fragments. Now, they are just a simple colour
 * based on the ship colour, since it was almost never possible to
 * see details anyway.
 */
class CellFragment : public GameObject {
  private:

  float theta, vtheta;

  float mass;
  GLuint vao, vbo;
  /* The vertices, initialized by getAreaAndSetVertices() and put
   * into the vbo by initVBO(). These are mutable so that they
   * can be set in the constructor.
   */
  mutable shader::quickV vertices[3];

  float lifeLeft;

  shader::quickU colour;

  /* Constructor is private. Call CellFragment::spawn(Cell*) instead.
   */
  CellFragment(GameField*, Cell*, float x, float y, float vx, float vy, const shader::quickU& colour);

  /* Besides returning the area, it also sets the vertices. */
  float getAreaAndSetVertices() const noth;

  GLuint initVBO() const noth;

  public:
  virtual ~CellFragment();
  /** Spawns a random number of CellFragments from the given Cell. */
  static void spawn(Cell*, Blast*) noth;

  virtual bool update(float) noth;
  virtual void draw() noth;

  virtual float getRadius() const noth { return STD_CELL_SZ/2; }
  virtual float getRotation() const noth { return theta; }

  virtual bool collideWith(GameObject*) noth;
  virtual CollisionResult checkCollision(GameObject*) noth { return NoCollision; }
};

#endif /*CELL_FRAGMENT_HXX_*/
