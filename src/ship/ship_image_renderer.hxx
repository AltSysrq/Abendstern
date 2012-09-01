/**
 * @file
 * @author Jason Lingle
 * @date 2012.07.22
 * @brief Contains the ShipImageRenderer class.
 */
#ifndef SHIP_IMAGE_RENDERER_HXX_
#define SHIP_IMAGE_RENDERER_HXX_

#include <GL/gl.h>

#include "src/core/aobject.hxx"

class Ship;

/**
 * Provides a facility to render a Ship object into a PNG image.
 */
class ShipImageRenderer: public AObject {
  Ship*const ship;
  unsigned currentCell;
  unsigned imgW, imgH, imgW2, imgH2;
  GLuint texture;

  ShipImageRenderer(Ship*);

public:
  virtual ~ShipImageRenderer();

  /**
   * Creates a ShipImageRenderer for the given Ship, if possible.
   * @return A ShipImageRenderer if the operation is supported, or NULL
   * otherwise.
   */
  static ShipImageRenderer* create(Ship*);

  /**
   * Renders the next Cell in the Ship.
   *
   * @return True iff a Cell was actually rendered. False indicates that
   * rendering is done, or an error occurred.
   */
  bool renderNext();

  /**
   * Saves the image to the given file, in PNG format.
   *
   * @return Whether the operation succeeded.
   */
  bool save(const char*) const;
};

#endif /* SHIP_IMAGE_RENDERER_HXX_ */
