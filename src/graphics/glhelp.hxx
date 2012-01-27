/**
 * @file
 * @author Jason Lingle
 * @brief Contains some simple functions to make working with OpenGL easier
 */

/*
 * glhelp.hxx
 *
 *  Created on: 23.01.2011
 *      Author: jason
 */

#ifndef GLHELP_HXX_
#define GLHELP_HXX_

#include <GL/gl.h>

#include "vaoemu.hxx"

/** Creates a new, single VAO and returns it. Aborts the program
 * if it fails.
 */
GLuint newVAO();

/** Creates a new, single VBO and returns it. Aborts the program
 * if it fails.
 */
GLuint newVBO();

#endif /* GLHELP_HXX_ */
