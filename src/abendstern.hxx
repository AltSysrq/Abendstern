/**
 * @file
 * @author Jason Lingle
 * @date 2012.05.20
 * @brief Provides access to core ("kernel") functionality implemented in
 * abendstern.cxx, such as the preliminary run mode.
 */
#ifndef ABENDSTERN_HXX_
#define ABENDSTERN_HXX_

/**
 * The possible OpenGL builds.
 */
enum AbendsternGLType { AGLT14=0, AGLT21, AGLT32 };
#if defined(AB_OPENGL_14)
#define THIS_GL_TYPE AGLT14
#elif defined(AB_OPENGL_21)
#define THIS_GL_TYPE AGLT21
#else
///This OpenGL build
#define THIS_GL_TYPE AGLT32
#endif

///In the preliminary configuration, the recommended OpenGL build to switch to
extern AbendsternGLType recommendedGLType;

/**
 * Set to true iff running in preliminary run mode.
 *
 * Preliminary run mode, initiated with the -prelim or -prelimauto flags, is a
 * special mode intended to allow the user to configure Abendstern before
 * starting the game proper.
 */
extern bool preliminaryRunMode;

/**
 * Restarts Abendstern and starts the GL version indicated by
 * recommendedGLType.
 *
 * This call will never return.
 */
void exitPreliminaryRunMode();

#endif /* ABENDSTERN_HXX_ */
