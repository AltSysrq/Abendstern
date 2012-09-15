/**
 * @file
 * @author Jason Lingle
 * @brief Contains the RenderQuad class
 */

/*
 * render_quad.hxx
 *
 *  Created on: 29.01.2011
 *      Author: jason
 */

#ifndef RENDER_QUAD_HXX_
#define RENDER_QUAD_HXX_

#include <GL/gl.h>
#include <vector>

#include "src/opto_flags.hxx"

class Ship;
class Cell;

/**
 * Manages a single texture tile for Ship graphics
 * (in the OpenGL3.2 version).
 *
 * To maximise performance, Ships are prerendered
 * into 256x256 "indexed" textures that are properly
 * coloured by the shader; a separate prerendered texture
 * (which is also 256x256, but a bitmap) holds the damage
 * information.
 *
 * This class primarily manages the lower-level resources,
 * such as vertex arrays.
 *
 * @see src/ship/ship_renderer.hxx
 */
class RenderQuad {
  GLuint vao, vbo;
  GLuint shipTex, damTex;
  bool hasVao, hasTex,
    shipTexValid, damTexValid,
    shipTexReady, damTexReady;

  const unsigned detailLevel;
  //This rectangle assumes that the ship is zeroed, and
  //is therefore never modified
  //(By zeroed, we mean the definition that ShipRenderer
  //has, where the bottom-left is at 0,0).
  //These are also the drawing boundaries.
  const float minX, minY, maxX, maxY;

  /* The list of cells that affect this quad. */
  std::vector<Cell*> cells;

  public:
  /** Instantiates the RenderQuad at the specified grid location.
   * No resources are allocated.
   */
  RenderQuad(unsigned detail, unsigned x, unsigned y);

  /** Deallocates any resources used by the RenderQuad.
   */
  ~RenderQuad();

  /** Frees all currently-used resources.
   * The quad may still be used after this, although it will
   * not be ready for rendering.
   */
  void free() noth;

  /** Returns true if the given Cell shows up in this quad,
   * and additionally adds it to the internal list.
   */
  bool doesCellIntersect(Cell*) noth;

  /**
   * Returns true of the quad is valid.
   * "Valid" indicates that all textures exist and can be drawn properly, but
   * might not yet be up-to-date.
   */
  bool isValid() const noth {
    if (cells.empty()) return true;
    return hasVao && hasTex && shipTexValid && damTexValid;
  }

  /** Returns true if the quad is ready for rendering.
   * "Ready for rendering" indicates that both the vertex array
   * AND the textures are up-to-date.
   * An empty quad is always ready, since there is nothing to do.
   */
  bool isReady() const noth {
    if (cells.empty()) return true;
    return isValid() && shipTexReady && damTexReady;
  }
  /** Performs any setup necessary to make the quad ready for rendering.
   * "Ready for rendering" indicates that both the vertex array
   * AND the textures are up-to-date.
   * This will allocate any resources necessary to perform rendering.
   * The two arguments are X and Y transformations to perform to properly
   * zero the ship's location.
   */
  void makeReady(float,float) noth;

  /** Indicates that the damage information in one or more of the cells
   * that affect this quad has changed. This invalidates, but does not
   * free, the damage texture. After this call, the quad is not
   * ready for rendering.
   */
  void cellDamaged() noth;

  /** Indicates that a significant physical alteration occurred.
   * Both the ship and damage textures are invalidated, but not
   * freed. After this call, the quad is not ready for rendering.
   * If the cell argument is non-NULL, that cell is removed from
   * the list of affecting cells. If this leaves the quad with
   * no cells, free() is automatically called.
   */
  void physicalChange(const Cell*) noth;

  /** Draws the quad. This function assumes both that the shader
   * has been activated appropriately, and that the quad is
   * ready for rendering. The effects of this function when
   * these conditions are not met are undefined, as no checks
   * for readiness are made.
   */
  void draw() const noth;
};
#endif /* RENDER_QUAD_HXX_ */
