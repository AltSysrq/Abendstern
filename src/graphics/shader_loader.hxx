/**
 * @file
 * @author Jason Lingle
 * @brief Contains macros that simplify Shader loading.
 *
 * This file defines a pair of macros that make on-demand
 * initialization of shaders easy. Use the following code:
 * \verbatim
 *   DELAY_SHADER(name)
 *   shader constructor arguments (naked)
 *   END_DELAY_SHADER(name);
 * \endverbatim
 * You can now access the shader with name->whatever. Arbitrary
 * modifiers may be placed before name (ie, "static foo").
 *
 * The macro VIRTUAL_SHADER may be defined to "virtual" allow having an
 * extern ShaderDelayLoader& referencing a loader in another
 * file.
 */

/*
 * shader_loader.hxx
 *
 *  Created on: 21.01.2011
 *      Author: jason
 */

#ifndef SHADER_LOADER_HXX_
#define SHADER_LOADER_HXX_

#include <new> //for placement version

/** The base class for delayed shader loaders.
 * @see Long description of src/graphics/shader_loader.hxx
 */
class ShaderDelayLoader {
  protected:
  bool isLoaded;
  ShaderDelayLoader()
  : isLoaded(false)
  { }
  public:
  virtual const Shader* operator->() = 0;
  /** For easter eggs only.
   *
   * Does not actually unload the shader, only causes
   * it to be loaded again.
   */
  void unload() { isLoaded = false; }
};

/**
 * Redefine this to virtual to enable using a generic ShaderDelayLoader
 * to reference the generated class.
 * (Currently, this actually makes no difference, but it should be used
 * as such in case of future changes).
 */
#define VIRTUAL_SHADER

#ifdef AB_OPENGL_14
  #define DELAY_SHADER_CONVERT_FIRST_ARGUMENT(x) gl32emu::SE_##x
#else
  /** Used internally. */
  #define DELAY_SHADER_CONVERT_FIRST_ARGUMENT(x) #x
#endif

/* Originally, we did not take a parameter for DELAY_SHADER,
 * and the class was anonymous. However, it appears the
 * MSVC++2010 linker can't tell the difference between multiple
 * anonymous classes with the same base class.
 * Therefore, use the shader name as part of a class name.
 */
/** Begins a new delayed shader loader with no auxilliary linker function */
#define DELAY_SHADER(x) DELAY_SHADER_AUX(x,NULL)
/** Begins a new delayed shader loader with the given auxilliary linker function */
#define DELAY_SHADER_AUX(x,aux) \
  class DelayShader_##x: public ShaderDelayLoader {\
    char raw[sizeof(Shader)];\
    public:\
    VIRTUAL_SHADER const Shader* operator->() {\
      if (!isLoaded) {\
        new ((Shader*)raw) Shader(DELAY_SHADER_CONVERT_FIRST_ARGUMENT(x), aux,
/** Terminates a delayed shader loader */
#define END_DELAY_SHADER(name)    );\
        isLoaded=true;\
      }\
      return (const Shader*)raw;\
    }\
  } name

#endif /* SHADER_LOADER_HXX_ */
