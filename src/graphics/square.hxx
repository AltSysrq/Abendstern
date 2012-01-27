/**
 * @file
 * @author Jason Lingle
 * @brief Contains a simple interface for drawing squares.
 */

/*
 * square.hxx
 *
 *  Created on: 25.01.2011
 *      Author: jason
 */

#ifndef SQUARE_HXX_
#define SQUARE_HXX_

#include <GL/gl.h>

/**
 * Contains functions for drawing squares.
 * Many graphics draw a simple square, with texture coordinates
 * from 0 to 1, using a shader to handle the interior. The functions
 * in this file are used to eliminate the redundancy of each graphic
 * handling these itself.
 *
 * This is compatible with any standard Abendstern shader that uses
 * the shader::VertTexc for vertices.
 */
namespace square_graphic {
  /** Binds the VAO and VBO. If they have not yet been created,
   * they are now. There is no need to call Shader::setupVBO()
   * manually.
   */
  void bind();

  /** Draw the square. This is a simple convenience function.
   * bind() must have been called recently enough.
   */
  static inline void draw() {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
}

#endif /* SQUARE_HXX_ */
