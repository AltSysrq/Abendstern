/**
 * @file
 * @author Jason Lingle
 * @brief Contains the LightTrail class
 */

/*
 * light_trail.hxx
 *
 *  Created on: 11.02.2011
 *      Author: jason
 */

#ifndef LIGHT_TRAIL_HXX_
#define LIGHT_TRAIL_HXX_

#include <GL/gl.h>
#include <vector>

#include "src/graphics/vec.hxx"
#include "src/sim/game_object.hxx"

/** A LightTrail is an effects object that uses the trail stack to
 * display a trail of light. It is updated solely on the GPU.
 * The LightTrail does not need to refresh the buffer in its entirety
 * every time an update occurs, and uniforms must be set only once
 * per object.
 *
 * LightTrails will cease to exist if they have not been drawn for
 * a few seconds.
 */
class LightTrail: public GameObject {
  public:
  struct Uniform {
    vec4 baseColour;
    vec4 colourFade;
    float currentTime;
    float baseWidth;
  };

  struct Vertex {
    #ifdef AB_OPENGL_21
    float ix;
    #endif
    vec2 vertex;
    vec2 velocity;
    float creation;
    float isolation;
    float expansion;
  };

  private:
  Uniform uniform;

  /* The resolution indicates how many vertices exist in
   * the line strip of the trail.
   */
  const unsigned resolution;

  /* This is the time of creation, which we use to offset
   * the game clock so we don't lose FP precision with
   * really large numbers.
   */
  const unsigned long creationTime;

  const float lifeTime;

  /* This vector contains an array of chars, fixed to size
   * resolution. If a given char is non-zero, there is a
   * vertex to be drawn here. chars are used instead of bools
   * to avoid the bitset-like behaviour in that case, which
   * is slower.
   */
  std::vector<char> usedVertices;

  /* The next vertex to add. Only well-defined if
   * nextVertexStatus is not None.
   */
  Vertex nextVertex;

  /* The status of the next vertex. */
  enum {
    /* There is no new vertex to add. */
    None,
    /* No new vertex has been specified, but
     * if none is by the time to actually add it,
     * we copy the previous as isolated, so that
     * the head smoothly blends out.
     */
    Isolated,
    /* A new vertex is ready. Note that we draw
     * it isolated if it is the first in its
     * chain even in this case.
     */
    Complete
  } nextVertexStatus;

  /* How much time between adding new vertices. */
  const float timeBetweenVertexAdd;

  /* How much time until we add the next vertex. */
  float timeUntilVertexAdd;

  /* The index of the /current/ vertex. Before we add
   * a new vertex, this is incremented, modulus resolution.
   */
  unsigned currentVertex;

  /* How long since the last call to draw() */
  float timeSinceDraw;

  GLuint vao, vbo, vio;

  public:
  /**
   * Constructs a new LightTrail with the given parms.
   * @param field The GameField this LightTrail will live in
   * @param ttl Number of milliseconds a segment lives
   * @param resolution Number of segments
   * @param maxWidth Maximum width a segment reaches
   * @param baseWidth Initial width of a segment
   * @param baseR Initial red colour component
   * @param baseG Initial green colour component
   * @param baseB Initial blue colour component
   * @param baseA Initial alpha colour component
   * @param decayR Slope of red colour component, per ms
   * @param decayG Slope of green colour component, per ms
   * @param decayB Slope of blue colour component, per ms
   * @param decayA Slope of alpha colour component, per ms
   */
  LightTrail(GameField* field, float ttl, unsigned resolution,
             float maxWidth, float baseWidth,
             float baseR, float baseG, float baseB, float baseA,
             float decayR, float decayG, float decayB, float decayA);
  virtual ~LightTrail();

  virtual bool update(float) noth;
  virtual void draw() noth;
  virtual float getRadius() const noth;

  /** Alters the maximum width */
  void setWidth(float maxWidth) noth;

  /** Emits a new vertex, and updates position to this location.
   *
   * @param x New X coord
   * @param y New Y coord
   * @param vx New X velocity
   * @param vy New Y velocity
   */
  void emit(float x, float y, float vx, float vy) noth;
};

#endif /* LIGHT_TRAIL_HXX_ */
