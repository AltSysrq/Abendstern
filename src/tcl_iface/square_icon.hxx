/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SquareIcon class.
 */

/*
 * square_icon.hxx
 *
 *  Created on: 11.09.2011
 *      Author: jason
 */

#ifndef SQUARE_ICON_HXX_
#define SQUARE_ICON_HXX_

#include <GL/gl.h>
#include <SDL.h>

#include "src/core/aobject.hxx"

/**
 * Simple class to load and draw a square icon.
 *
 * This is primarily intended for Tcl. It loads an image
 * from the specified file and displays it when requested.
 * Errors are silently ignored, resulting in a blank image.
 * When loading, the image may optionally be scaled to fit
 * the specified size. A facility for writing a scaled PNG
 * is also provided.
 */
class SquareIcon: public AObject {
  GLuint tex;
  SDL_Surface* surf;

  public:
  /** Describes the requirements for loading an image.
   *
   * If requirements are not met, the image is not loaded.
   */
  enum LoadReq {
    Strict, ///< Both width and height must match the specified size exactly
    Lax, ///< Allows dimenions to be less than the specified size
    Scale ///< Any size is accepted; the result may be scaled down if needed
  };

  /** Constructs a new, empty SquareIcon. */
  SquareIcon();//!< SquareIcon
  virtual ~SquareIcon();

  /** Attempts to load an icon from the given source.
   *
   * @param filename The filename to load from
   * @param size The expected size for both dimensions of the image
   * @param req The requirements to accept the image
   * @return True if the operation succeeds, false otherwise
   * @see LoadReq
   */
  bool load(const char* filename, unsigned size, LoadReq req);

  /** Discards any image that may be in memory. */
  void unload();

  /** Draws the image, if there is any.
   *
   * The image spans from (0,0) to (1,1).
   */
  void draw() const;

  /** Returns whether an image is currently loaded. */
  bool isLoaded() const;

  /** Saves the currently-loaded image under the requested filename.
   *
   * This is mainly useful when used with Scale.
   *
   * @param filename The file to write to
   * @return True if the operation succeeds, false otherwise
   */
  bool save(const char* filename) const;
};

#endif /* SQUARE_ICON_HXX_ */
