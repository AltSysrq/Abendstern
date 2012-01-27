/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Shader class
 */

/*
 * shader.hxx
 *
 *  Created on: 21.01.2011
 *      Author: jason
 */

#ifndef SHADER_HXX_
#define SHADER_HXX_

#include <cstdarg>
#include <cstddef>
#include <vector>
#include <GL/gl.h>
#include "gl32emu.hxx"
#include "vaoemu.hxx"

/** Enables the LSD-mode Easter Egg.
 * Any shaders already used must be reloaded.
 */
void enableLSDMode();
/** Returns whether LSD-mode is forced. */
bool isLSDModeForced();

#ifndef AB_OPENGL_14
/**
 * The Shader class is a wrapper for interfacing with OpenGL shaders,
 * including uniform access and vertex formatting.
 *
 * In OpenGL 2.1 compatibility mode, Shader is a sort of dummy class that
 * instructs gl32emu to emulate a set of supported shader types.
 */
class Shader {
  public:
  /** Defines the types of uniforms that are supported. */
  enum UniformType { Float, FloatArray, Integer, Vec2, Vec3, Vec4, Matrix };

  private:
  GLuint shaderID;

  struct Uniform {
    GLint id;
    UniformType type;
    unsigned offset;
    unsigned arrayLength;
  };

  std::vector<Uniform> uniforms;

  struct VertexFormat {
    //ID is implicit from the index in the vector
    unsigned offset;
    GLuint nfloats;
  };
  GLsizei vertexSize;
  std::vector<VertexFormat> vertexFormat;

  bool automaticTransform;
  GLint transform;

  public:
  /** Construct the Shader.
   * The full argument format is:
   * \verbatim
   * const char*        name
   * void(*)(GLuint)    May be null. This function is called immediately before linking,
   *                    with the program number passed.
   * size_t             vertexSize
   * {
   *   const char*      attribute name
   *   size_t           datum offset
   *   size_t           number of floats in vertex
   * }...
   * NULL
   * bool               If true, handle "transform" matrix uniform
   *                    automatically
   * {
   *   const char*      uniform name
   *   UniformType      uniform type
   *   size_t           datum offset
   *   (only if type is FloatArray:
   *   size_t           array length)
   * }
   * NULL
   * \endverbatim
   *
   * If the shader cannot be loaded, error details are dumped and
   * the program exits.
   */
  Shader(const char*, void (*)(GLuint), std::size_t, ...);

  ~Shader();

  /** Sets the current VB0 up for the configured
   * vertex struct.
   */
  void setupVBO() const;

  /** Activates the shader, calling setUniforms
   * immediately. After this call, it is possible
   * to make drawing calls.
   */
  void activate(const void* uniforms) const;

  /** Resets the uniforms for the shader. This
   * may be called ONLY if this is the current
   * shader.
   */
  void setUniforms(const void*) const;
};

/* The below macros are intended to simplify calling the
 * Shader constructor.
 */

/**
 * With a Shader constructor call, declares an attribute that is not a float.
 * Expands into a list of three arguments, to be followed by a comma,
 * indicating the name, offset, and nfloat arguments for one attribute.
 * Must have defined VERTEX_TYPE before.
 */
#define VATTRIB(name) #name, offsetof(VERTEX_TYPE, name), sizeof(((VERTEX_TYPE*)NULL)->name.v)/sizeof(float)
/**
 * Within a Shader constructor call, declares a float attribute.
 * Same as VATTRIB, but works with a single float.
 * Must have defined VERTEX_TYPE before.
 */
#define VFLOAT(name) #name, offsetof(VERTEX_TYPE, name), 1
/**
 * Within a Shader constructor call, declares a uniform that is a float.
 * Expands into a list of three arguments, to be followed by a comma,
 * indicating the name, type, and offset for one uniform.
 * Must have defined UNIFORM_TYPE before.
 */
#define UNIFLOAT(name) #name, Shader::Float, offsetof(UNIFORM_TYPE,name)
/**
 * Within a Shader constructor call, declares a uniform that is a float array.
 * Expands into a list of four arguments, to be followed by a comma,
 * indicating the name, type, offset, and size for a float array uniform.
 * Must have defined UNIFORM_TYPE before.
 */
#define UNIFLOATA(name) #name, Shader::FloatArray, offsetof(UNIFORM_TYPE,name),\
  sizeof(((UNIFORM_TYPE*)NULL)->name)/sizeof(float)
/**
 * Within a Shader constructor call, declares a uniform of any non-float,
 * non-float-array type.
 * Automatically detects types Integer, Vec2, Vec3, Vec4, Matrix.
 * Expands into a list of three arguments, to followed by a comma,
 * indicating the name, type, offset, and size of one of the supported
 * types of uniforms.
 * Must have defined UNIFORM_TYPE before.
 */
#define UNIFORM(name) #name,\
  sizeof(((UNIFORM_TYPE*)NULL)->name) == sizeof(int)? Shader::Integer\
: sizeof(((UNIFORM_TYPE*)NULL)->name) == sizeof(vec2)? Shader::Vec2\
: sizeof(((UNIFORM_TYPE*)NULL)->name) == sizeof(vec3)? Shader::Vec3\
: sizeof(((UNIFORM_TYPE*)NULL)->name) == sizeof(vec4)? Shader::Vec4\
: Shader::Matrix, offsetof(UNIFORM_TYPE,name)

#else /* defined(AB_OPENGL_14) */
class Shader {
  unsigned vertexOffset, vertexNFloats, colourOffset, colourNFloats, texcooOffset, texcooNFloats;
  unsigned vertexSize;

  unsigned unicolourOffset, unicolourNFloats;

  gl32emu::ShaderEmulation emulation;

  public:
  /* Usage:
   * emulation, ignored, vertexSize,
   * unsigned: next attribute type
   *   NULL:    None
   *   1        vertex
   *   2        colour
   *   3        texCoord
   *   vertex (only if former non-NULL):
   *     unsigned       offset
   *     unsigned       nfloats
   * bool       must be true
   * int        next uniform information:
   *   NULL     End of uniforms
   *   1        Colour argument, followed by
   *     unsigned       offset
   *     unsigned       nfloats
   *   2        Ignore
   */
  Shader(gl32emu::ShaderEmulation, void(*)(GLuint), unsigned vertexSize, ...);
  ~Shader();
  void setupVBO() const;
  void activate(const void*) const;
  void setUniforms(const void*) const;
};

#define VATTRIB(name) GL32EMU_VERTEX_ATTRIB_##name, offsetof(VERTEX_TYPE, name), sizeof(((VERTEX_TYPE*)NULL)->name)/sizeof(float)
#define UNIFORM(name) GL32EMU_UNIFORM_##name, offsetof(UNIFORM_TYPE, name), sizeof(((UNIFORM_TYPE*)NULL)->name)/sizeof(float)

#define GL32EMU_VERTEX_ATTRIB_vertex 1
#define GL32EMU_VERTEX_ATTRIB_colour 2
#define GL32EMU_VERTEX_ATTRIB_texCoord 3
#define GL32EMU_UNIFORM_colour 1
#define GL32EMU_UNIFORM_modColour 1
#define GL32EMU_UNIFORM_ignore 2
#define GL32EMU_UNIFORM_colourmap GL32EMU_UNIFORM_ignore
#define GL32EMU_UNIFORM_colourMap GL32EMU_UNIFORM_ignore

#endif /* else defined(AB_OPENGL_14) */

#endif /* SHADER_HXX_ */
