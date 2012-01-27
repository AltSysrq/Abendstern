/**
 * @file
 * @author Jason Lingle
 * @brief Defines several generic shaders used commonly throughout
 * Abendstern.
 */

/*
 * cmn_shaders.hxx
 *
 *  Created on: 22.01.2011
 *      Author: jason
 */

#ifndef CMN_SHADERS_HXX_
#define CMN_SHADERS_HXX_

#include <GL/gl.h>

#include "vec.hxx"
#include "mat.hxx"
#include "shader.hxx"
#include "shader_loader.hxx"

namespace shader {
  struct Vert {
    vec4 vertex;
  };
  struct Vert2 {
    vec2 vertex;
  };
  struct VertCol {
    vec4 vertex, colour;
  };
  struct VertTexc {
    vec4 vertex;
    vec2 texCoord;
  };
  struct Texture {
    GLuint colourMap;
    vec4 modColour;
  };
  struct Colour {
    vec4 colour;
  };
  struct TexScreen {
    GLuint colourMap;
    vec4 modColour;
    int screenW, screenH;
  };
  struct ColScreen {
    vec4 colour;
    int screenW, screenH;
  };

  /** Basic colour shader. Supports blending.
   * Use VertCol vertices, NULL uniform.
   * Automatic transform.
   */
  extern ShaderDelayLoader& basic;
  typedef VertCol basicV;
  typedef void    basicU;

  /** Quick, single-colour shader.
   * Use Vert vertices, Colour uniform.
   * Automatic transform. Vertices are
   * strictly two-dimensional.
   */
  extern ShaderDelayLoader& quick;
  typedef Vert2  quickV;
  typedef Colour quickU;

  /** Texture modulating shader.
   * Use VertTexc vertices, Texture
   * uniform. Automatic transform.
   */
  extern ShaderDelayLoader& textureModulate;
  typedef VertTexc textureModulateV;
  typedef Texture  textureModulateU;

  /** Texture replacing shader.
   * Use VertTexc vertices, Texture uniform.
   * Automatic transform.
   */
  extern ShaderDelayLoader& textureReplace;
  typedef VertTexc textureReplaceV;
  typedef Texture  textureReplaceU;

  /** Texture stensil shader.
   * Use VertTexc vertices, Texture uniform.
   * Automatic transform.
   */
  extern ShaderDelayLoader& textureStensil;
  typedef VertTexc textureStensilV;
  typedef Texture  textureStensilU;

  /** Stiple solid colour shader.
   * Use Vert2 vertices, ColScreen uniform.
   * Automatic transform.
   */
  extern ShaderDelayLoader& colourStiple;
  typedef Vert2     colourStipleV;
  typedef ColScreen colourStipleU;

  /** Stiple texture stensil shader.
   * Use VertTexc vertices, TexScreen uniform.
   * Automatic transform.
   */
  extern ShaderDelayLoader& textureStensilStiple;
  typedef VertTexc  textureStensilStipleV;
  typedef TexScreen textureStensilStipleU;
}

#endif /* CMN_SHADERS_HXX_ */
