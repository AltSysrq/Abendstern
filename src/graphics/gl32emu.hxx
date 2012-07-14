/*
 * gl32emu.hxx
 *
 *  Created on: 15.02.2011
 *      Author: jason
 */

#ifndef GL32EMU_HXX_
#define GL32EMU_HXX_

#include <GL/gl.h>

/**
 * @file
 * @author Jason Lingle
 * @brief Defines the prototypes for Abendstern's
 * OpenGL3.2-on-OpenGL1.4 emulation layer.
 *
 * It emulates, with the help of the Shader class, the following
 * functionality:
 * + Vertex arrays
 * + Vertex buffer objects, both GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER
 * + Common shader types that are already implemented by the fixed-function
 *   pipeline
 *
 * The emulation implementation is intentionally incomplete and incompatible
 * unless the code-paths follow Abendstern convention. Most notably:
 * + Vertex array objects are meaningless; the current array is entirely ignored
 * + Element array buffers are also meaningless
 * + Buffer data cannot be modified on a partial basis
 * + Arrays cannot be rendered on a partial basis
 * + DrawElements is indistinguishable from DrawArrays
 *
 * The emulation supports the following vertex attributes:
 * + vertex, for position, as any size vector
 * + colour, for colour, as any size vector
 * + texCoord, for the texture coordinate, as any size vector
 *
 * Arrays are handled by deinterleaving the information and storing it in
 * a display list, which is what the VBO is actually binding to. The format
 * of index arrays is inferred by using the largest size (selected from
 * unsigned int, unsigned short, and unsigned char) that makes sense, tested
 * from largest to smallest. It is therefore important not to have reference
 * the zero index too often (namely, every other index or three out of four
 * indices, etc).
 *
 * The following shaders are supported via the fixed-function pipeline:
 * + quick
 * + simpleColour
 * + textureReplace
 * + textureModulate
 * + textureStensil (by texture modulation)
 * + stipleColour
 * + stipleTexture (modulate)
 */

#if defined(AB_OPENGL_14) || defined(DOXYGEN)

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 1
#endif

#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 2
#endif

/** Contains facilities for emulating OpenGL 3.2 on OpenGL1.4.
 * @see src/graphics/gl32emu.hxx
 */
namespace gl32emu {
  /** Supported shaders for emulation.
   * @see src/graphics/cmn_shaders.hxx
   */
  enum ShaderEmulation {
    SE_quick, SE_simpleColour, SE_textureReplace, SE_textureModulate,
    SE_textureStensil, SE_stipleColour, SE_stipleTexture,
  };

  /** Supported vertex attributes */
  enum VertexAttribute {
    Position, Colour, TextureCoordinate
  };

  /** Replacement for glGenVertexArrays */
  void genVertexArrays(unsigned, GLuint*);
  /** Replacement for glDeleteVertexArrays */
  void deleteVertexArrays(unsigned, const GLuint*);
  /** Replacement for glGenBuffers */
  void genBuffers(unsigned,GLuint*);
  /** Replacement for glDeleteBuffers */
  void deleteBuffers(unsigned, const GLuint*);
  /** Replacement for glBindVertexArray */
  void bindVertexArray(GLuint);
  /** Replacement for glBindBuffer */
  void bindBuffer(GLenum,GLuint);
  /** Replacement for glBufferData */
  void bufferData(GLenum,GLsizei,const GLvoid*,GLenum);
  /** Replacement for glDrawArrays */
  void drawArrays(GLenum,GLuint,GLuint);

  /** Sets the size of a vertex in the buffer.
   * For use by shaders to specify buffer format.
   */
  void setBufferVertexSize(unsigned);

  /** Adds information about buffer format.
   * Specify the offset, size in floats, and type of an attribute.
   * @param off Offset from a vertex of the attribute
   * @param nfloats The number of floating-point values in the attribute
   * @param attr The type of the attribute
   */
  void setBufferAttributeInfo(unsigned off, unsigned nfloats, VertexAttribute attr);

  /** Sets the current shader emulation mode. */
  void setShaderEmulation(ShaderEmulation);

  /** Initialises the emulation layer.
   * This MUST be called before any drawing may be done.
   */
  void init();
}

//Stomp our emulation functions on top of anything that the normal OpenGL
//API may have defined
//But don't do this if Doxygen is running over us
#ifndef DOXYGEN
#define glGenVertexArrays gl32emu::genVertexArrays
#define glDeleteVertexArrays gl32emu::deleteVertexArrays
#define glGenBuffers gl32emu::genBuffers
#define glDeleteBuffers gl32emu::deleteBuffers
#define glBindVertexArray gl32emu::bindVertexArray
#define glBindBuffer gl32emu::bindBuffer
#define glBufferData gl32emu::bufferData
#define glDrawArrays gl32emu::drawArrays
#define glDrawElements(type, len, sz, off) glDrawArrays(type, off, len)

//Define some constants that might not normally be defined (on pure 2.1-- systems)
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0
#define GL_DYNAMIC_DRAW 0
#define GL_STREAM_DRAW 0
#endif /* DOXYGEN */
#endif

#endif /* AB_OPENGL_14 */

#endif /* GL32EMU_HXX_ */
