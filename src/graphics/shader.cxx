/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/shader.hxx
 */

/*
 * shader.cxx
 *
 *  Created on: 21.01.2011
 *      Author: jason
 */

#if !defined(DEBUG) && !defined(AB_OPENGL_14)
#define NDEBUG
#endif /* DEBUG */

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdarg>
#include <cstdlib>
#include <cassert>

#include <libconfig.h++>

#include <GL/gl.h>

#include "vaoemu.hxx"
#include "shader.hxx"
#include "vec.hxx"
#include "mat.hxx"
#include "matops.hxx"
#include "src/secondary/confreg.hxx"
#include "src/globals.hxx"
#include "src/exit_conditions.hxx"

using namespace std;
using namespace libconfig;

static bool forceLSDMode = false;
void enableLSDMode() {
  forceLSDMode = true;
}
bool isLSDModeForced() {
  return forceLSDMode;
}

#ifndef AB_OPENGL_14

Shader::Shader(const char* name, void (*prelink)(GLuint), size_t vertexSize, ...) {
  string vertexShaderSource, fragShaderSource, geoShaderSource;
  GLuint vertexShader, fragShader, geoShader=0;
  bool hasFragShader, hasGeoShader;
  #ifndef AB_OPENGL_21
  const char* profile = (forceLSDMode? "lsd" : conf["conf"]["graphics"]["shader_profile"]);
  #else
  const char* profile = "gl21";
  #endif

  try {
    //This line is the only one here that can throw an exception
    Setting& stack(conf["shaders"][profile][name]);
    if (stack.getLength() == 2) hasGeoShader=false;
    else if (stack.getLength() == 3) hasGeoShader=true;
    else {
      cerr << "FATAL: Malformed shader stack: " << name << endl;
      exit(EXIT_MALFORMED_CONFIG);
    }

    {
      string filename="shaders/";
      filename += (const char*)stack[0];
      filename += ".glsl";
      ifstream in(filename.c_str());
      getline(in, vertexShaderSource, '\0');
      if (!in) {
        cerr << "FATAL: Unable to load vertex shader source." << endl;
        cerr << "Stack: " << name << ", file: " << filename << endl;
        exit(EXIT_MALFORMED_DATA);
      }
    }

    {
      string filename="shaders/";
      hasFragShader = (bool)strlen(stack[1]);
      if (hasFragShader) {
        filename += (const char*)stack[1];
        filename += ".glsl";
        ifstream in(filename.c_str());
        getline(in, fragShaderSource, '\0');
        if (!in) {
          cerr << "FATAL: Unable to load fragment shader source." << endl;
          cerr << "Stack: " << name << ", file: " << filename << endl;
          exit(EXIT_MALFORMED_DATA);
        }
      }
    }

    if (hasGeoShader) {
      #ifndef AB_OPENGL_21
      string filename="shaders/";
      filename += (const char*)stack[2];
      filename += ".glsl";
      ifstream in(filename.c_str());
      getline(in, geoShaderSource, '\0');
      if (!in) {
        cerr << "FATAL: Unable to load geometry shader source." << endl;
        cerr << "Stack: " << name << ", file: " << filename << endl;
        exit(EXIT_MALFORMED_DATA);
      }
      #else
      cerr << "FATAL: Shader stack " << name << " has geometry shader in GL 2.1 mode." << endl;
      exit(EXIT_PROGRAM_BUG);
      #endif
    }
  } catch (...) {
    cerr << "FATAL: No such shader stack: " << name << endl;
    exit(EXIT_PROGRAM_BUG);
  }

  //OK, begin creating the shaders
  vertexShader=glCreateShader(GL_VERTEX_SHADER);
  if (hasFragShader)
    fragShader=glCreateShader(GL_FRAGMENT_SHADER);
  if (hasGeoShader)
    geoShader=glCreateShader(GL_GEOMETRY_SHADER);

  //Upload source
  const char* tmp;
  glShaderSource(vertexShader, 1, &(tmp=vertexShaderSource.c_str()), NULL);
  if (hasFragShader)
    glShaderSource(fragShader, 1, &(tmp=fragShaderSource.c_str()), NULL);
  if (hasGeoShader)
    glShaderSource(geoShader, 1, &(tmp=geoShaderSource.c_str()), NULL);

  //Compile
  glCompileShader(vertexShader);
  if (hasFragShader)
    glCompileShader(fragShader);
  if (hasGeoShader)
    glCompileShader(geoShader);

  //Check for errors
  GLint err;
  bool errors=false;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &err);
  if (err == GL_FALSE) {
    errors=true;
    char why[1024];
    glGetShaderInfoLog(vertexShader, 1024, NULL, why);
    cerr << "Unable to compile vertex shader for: " << name << endl;
    cerr << why << endl;
  }
  if (hasFragShader)
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &err);
  if (err == GL_FALSE) {
    errors=true;
    char why[1024];
    glGetShaderInfoLog(fragShader, 1024, NULL, why);
    cerr << "Unable to compile fragment shader for: " << name << endl;
    cerr << why << endl;
  }

  err=GL_TRUE;
  if (hasGeoShader)
    glGetShaderiv(geoShader, GL_COMPILE_STATUS, &err);
  if (err==GL_FALSE) {
    errors=true;
    char why[1024];
    glGetShaderInfoLog(geoShader, 1024, NULL, why);
    cerr << "Unable to compile geometry shader for: " << name << endl;
    cerr << why << endl;
  }

  if (errors) {
    cerr << "FATAL: Unable to load shaders." << endl;
    exit(EXIT_MALFORMED_DATA);
  }

  //Begin linking
  shaderID = glCreateProgram();
  glAttachShader(shaderID, vertexShader);
  if (hasGeoShader)
    glAttachShader(shaderID, geoShader);
  if (hasFragShader)
    glAttachShader(shaderID, fragShader);

  //Finish linking
  if (prelink) prelink(shaderID);

  //Attributes
  this->vertexSize=vertexSize;
  va_list args;
  va_start(args, vertexSize);

  const char* attrName;
  size_t attrOff, attrNFloat;
  unsigned attrIx=0;
  for (attrName=va_arg(args, const char*); attrName; attrName=va_arg(args, const char*)) {
    attrOff=va_arg(args, size_t);
    attrNFloat=va_arg(args, size_t);
    VertexFormat vf = { attrOff, attrNFloat };
    vertexFormat.push_back(vf);
    glBindAttribLocation(shaderID, attrIx++, attrName);
  }

  glLinkProgram(shaderID);
  //Make sure it worked
  glGetProgramiv(shaderID, GL_LINK_STATUS, &err);
  if (err == GL_FALSE) {
    char why[1024];
    glGetProgramInfoLog(shaderID, 1024, NULL, why);
    cerr << "FATAL: Unable to link shader: " << name << endl;
    cerr << why << endl;
    exit(EXIT_MALFORMED_DATA);
  }

  //Free now unneeded programs
  glDeleteShader(vertexShader);
  if (hasFragShader)
    glDeleteShader(fragShader);
  if (hasGeoShader)
    glDeleteShader(geoShader);

  //Finish constructing self
  //Read in uniform information
  automaticTransform=va_arg(args, int);
  if (automaticTransform) {
    transform=glGetUniformLocation(shaderID, "transform");
    if (transform == -1) {
      cerr << "FATAL: Unable to find uniform transform for: " << name << endl;
      exit(EXIT_MALFORMED_DATA);
    }
  }

  const char* uniName;
  UniformType uniType;
  size_t uniOff=0, uniASz=0;
  for (uniName=va_arg(args, const char*); uniName; uniName=va_arg(args, const char*)) {
    uniType=(UniformType)va_arg(args, int);
    uniOff=va_arg(args, size_t);
    if (uniType == FloatArray)
      uniASz=va_arg(args, size_t);

    GLint id=glGetUniformLocation(shaderID, uniName);
    if (id == -1) {
      //Missing uniform
      if (conf["shaders"]["optional_uniforms"].exists(uniName)) continue;
      cerr << "FATAL: Unable to find uniform " << uniName << " for: " << name << endl;
      exit(EXIT_MALFORMED_DATA);
    }

    Uniform u = { id, uniType, uniOff, uniASz };
    uniforms.push_back(u);
  }

  va_end(args);
}

Shader::~Shader() {
  glDeleteProgram(shaderID);
}

void Shader::setupVBO() const {
  vaoemu::resetVAO();
  for (unsigned i=0; i<vertexFormat.size(); ++i)
    glVertexAttribPointer(i, vertexFormat[i].nfloats, GL_FLOAT, GL_FALSE, vertexSize,
                          reinterpret_cast<const GLvoid*>(vertexFormat[i].offset));
  for (unsigned i=0; i<vertexFormat.size(); ++i)
    glEnableVertexAttribArray(i);
}

void Shader::activate(const void* uniData) const {
  glUseProgram(shaderID);
  setUniforms(uniData);
}

void Shader::setUniforms(const void* v_uniData) const {
  const char* uniData=(const char*)v_uniData;

  if (automaticTransform)
    glUniformMatrix4fv(transform, 1, GL_FALSE, mGet());

  for (unsigned i=0; i<uniforms.size(); ++i) switch (uniforms[i].type) {
    case Float:
      glUniform1f(uniforms[i].id, *(const float*)(uniData+uniforms[i].offset));
      break;
    case FloatArray:
      glUniform1fv(uniforms[i].id, uniforms[i].arrayLength,
          const_cast<GLfloat*>((const GLfloat*)(uniData + uniforms[i].offset)));
      break;
    case Integer:
      glUniform1i(uniforms[i].id, *(const int*)(uniData+uniforms[i].offset));
      break;
    case Vec2:
      glUniform2fv(uniforms[i].id, 1,
          const_cast<float*>(((const vec2*)(uniData+uniforms[i].offset))->v));
      break;
    case Vec3:
      glUniform3fv(uniforms[i].id, 1,
          const_cast<float*>(((const vec3*)(uniData+uniforms[i].offset))->v));
      break;
    case Vec4:
      glUniform4fv(uniforms[i].id, 1,
          const_cast<float*>(((const vec4*)(uniData+uniforms[i].offset))->v));
      break;
    case Matrix:
      glUniformMatrix4fv(uniforms[i].id, 1, GL_FALSE,
          const_cast<float*>(((const matrix*)(uniData+uniforms[i].offset))->v));
      break;
    default: assert(false);
  }
}

#else /* defined(AB_OPENGL_14) */
Shader::Shader(gl32emu::ShaderEmulation emul, void(*)(GLuint), unsigned vsz, ...)
: vertexNFloats(0), colourNFloats(0), texcooNFloats(0),
  vertexSize(vsz), unicolourNFloats(0), emulation(emul)
{
  va_list args;
  va_start(args, vsz);

  unsigned attribType;
  for (attribType = va_arg(args, unsigned); attribType; attribType = va_arg(args, unsigned)) {
    unsigned off, nf;
    off = va_arg(args, unsigned);
    nf = va_arg(args, unsigned);
    switch (attribType) {
      case GL32EMU_VERTEX_ATTRIB_vertex:
        vertexOffset=off;
        vertexNFloats=nf;
        break;
      case GL32EMU_VERTEX_ATTRIB_colour:
        colourOffset=off;
        colourNFloats=nf;
        break;
      case GL32EMU_VERTEX_ATTRIB_texCoord:
        texcooOffset=off;
        texcooNFloats=nf;
        break;
      default:
        cerr << "FATAL: Unknown vertex attribute type passed to shader emulator: " << attribType << endl;
        exit(EXIT_PROGRAM_BUG);
    }
  }

  bool b = va_arg(args, int);
  assert(b);

  unsigned uniType;
  for (uniType = va_arg(args, unsigned); uniType; uniType = va_arg(args, unsigned)) {
    unsigned off, nf;
    off = va_arg(args, unsigned);
    nf = va_arg(args, unsigned);
    switch (uniType) {
      case GL32EMU_UNIFORM_colour:
        unicolourOffset=off;
        unicolourNFloats=nf;
        break;
      case GL32EMU_UNIFORM_ignore: break;
      default:
        cerr << "FATAL: Unknown uniform type passed to shader emulator: " << uniType << endl;
        exit(EXIT_PROGRAM_BUG);
    }
  }

  va_end(args);
}

Shader::~Shader() {}

void Shader::setupVBO() const {
  gl32emu::setBufferVertexSize(vertexSize);
  gl32emu::setBufferAttributeInfo(vertexOffset, vertexNFloats, gl32emu::Position);
  gl32emu::setBufferAttributeInfo(colourOffset, colourNFloats, gl32emu::Colour);
  gl32emu::setBufferAttributeInfo(texcooOffset, texcooNFloats, gl32emu::TextureCoordinate);
}

void Shader::activate(const void* uni) const {
  gl32emu::setShaderEmulation(emulation);
  setUniforms(uni);
}

void Shader::setUniforms(const void* uni) const {
  const unsigned char* cuni=(const unsigned char*)uni;
  switch (unicolourNFloats) {
    case 3: glColor3fv((const float*)(cuni+unicolourOffset)); break;
    case 4: glColor4fv((const float*)(cuni+unicolourOffset)); break;
  }
}

#endif /* else define(AB_OPENGL_14) */
