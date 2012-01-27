/**
 * @file
 * @author Jason Lingle
 * @brief Interface to add Vertex Array Objects to OpenGL [ES] 2.0
 *
 * This file, when AB_OPENGL_21 is defined, provides the following
 * functions:
 *   glBindVertexArray
 *   glDeleteVertexArrays
 *   glGenVertexArrays
 * and replaces a number of OpenGL functions to support these.
 */

/*
 * vaoemu.hxx
 *
 *  Created on: 14.11.2011
 *      Author: jason
 */

#ifndef VAOEMU_HXX_
#define VAOEMU_HXX_

#include <GL/gl.h>

#ifdef AB_OPENGL_21
namespace vaoemu {
  void vertexAttribPointer(GLuint, GLuint, GLenum, GLboolean, GLsizei, const GLvoid*);
  void enableVertexAttribArray(GLuint);

  void bindVertexArray(GLuint);
  void genVertexArrays(GLsizei, GLuint*);
  void deleteVertexArrays(GLsizei, const GLuint*);
  void resetVAO();
  void bindBuffer(GLenum,GLuint);
}

#ifndef VAOEMU_CXX_
#define glVertexAttribPointer vaoemu::vertexAttribPointer
#define glEnableVertexAttribArray vaoemu::enableVertexAttribArray
#define glBindVertexArray vaoemu::bindVertexArray
#define glGenVertexArrays vaoemu::genVertexArrays
#define glDeleteVertexArrays vaoemu::deleteVertexArrays
#define glBindBuffer vaoemu::bindBuffer
#endif /* VAOEMU_CXX_ */
#else /* AB_OPENGL_21 */
namespace vaoemu {
  /** Resets a VAO for new setup.
   * This only has an effect in GL 21 mode.
   */
  static inline void resetVAO() {}
}
#endif /* AB_OPENGL_21 */

#endif /* VAOEMU_HXX_ */
