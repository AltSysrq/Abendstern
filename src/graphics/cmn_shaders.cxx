/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/cmn_shaders.hxx
 */

/*
 * cmn_shaders.cxx
 *
 *  Created on: 22.01.2011
 *      Author: jason
 */

#include <GL/gl.h>

#include "cmn_shaders.hxx"
#include "shader_loader.hxx"
#include "shader.hxx"
#include "vec.hxx"

namespace shader {
  #undef VIRTUAL_SHADER
  #define VIRTUAL_SHADER virtual
  #define VERTEX_TYPE basicV
  DELAY_SHADER(simpleColour)
    sizeof(basicV),
    VATTRIB(vertex), VATTRIB(colour),
    NULL,
    true,
    NULL
  END_DELAY_SHADER(static basic_);
  ShaderDelayLoader& basic(basic_);

  #undef VERTEX_TYPE
  #define VERTEX_TYPE quickV
  #define UNIFORM_TYPE quickU
  DELAY_SHADER(quick)
    sizeof(quickV),
    VATTRIB(vertex),
    NULL,
    true,
    UNIFORM(colour),
    NULL
  END_DELAY_SHADER(static quick_);
  ShaderDelayLoader& quick(quick_);

  #undef UNIFORM_TYPE
  #undef VERTEX_TYPE
  #define VERTEX_TYPE textureReplaceV
  #define UNIFORM_TYPE textureReplaceU
  DELAY_SHADER(textureReplace)
    sizeof(textureReplaceV),
    VATTRIB(vertex), VATTRIB(texCoord),
    NULL,
    true,
    UNIFORM(colourMap),
    NULL
  END_DELAY_SHADER(static textureReplace_);
  ShaderDelayLoader& textureReplace(textureReplace_);

  //Same types
  DELAY_SHADER(textureModulate)
    sizeof(textureModulateV),
    VATTRIB(vertex), VATTRIB(texCoord),
    NULL,
    true,
    UNIFORM(colourMap),
    UNIFORM(modColour),
    NULL
  END_DELAY_SHADER(static textureModulate_);
  ShaderDelayLoader& textureModulate(textureModulate_);

  //Same types
  DELAY_SHADER(textureStensil)
    sizeof(textureStensilV),
    VATTRIB(vertex), VATTRIB(texCoord),
    NULL,
    true,
    UNIFORM(colourMap),
    UNIFORM(modColour),
    NULL
  END_DELAY_SHADER(static textureStensil_);
  ShaderDelayLoader& textureStensil(textureStensil_);

  #undef VERTEX_TYPE
  #undef UNIFORM_TYPE
  #define VERTEX_TYPE colourStipleV
  #define UNIFORM_TYPE colourStipleU
  DELAY_SHADER(stipleColour)
    sizeof(colourStipleV),
    VATTRIB(vertex), NULL,
    true,
    UNIFORM(colour),
#ifndef AB_OPENGL_14
    UNIFORM(screenW), UNIFORM(screenH),
#endif
    NULL
  END_DELAY_SHADER(static colourStiple_);
  ShaderDelayLoader& colourStiple(colourStiple_);

  #undef VERTEX_TYPE
  #undef UNIFORM_TYPE
  #define VERTEX_TYPE textureStensilStipleV
  #define UNIFORM_TYPE textureStensilStipleU
  DELAY_SHADER(stipleTexture)
    sizeof(textureStensilStipleV),
    VATTRIB(vertex), VATTRIB(texCoord), NULL,
    true,
    UNIFORM(colourMap), UNIFORM(modColour),
#ifndef AB_OPENGL_14
    UNIFORM(screenW), UNIFORM(screenH),
#endif
    NULL
  END_DELAY_SHADER(static textureStensilStiple_);
  ShaderDelayLoader& textureStensilStiple(textureStensilStiple_);
}

