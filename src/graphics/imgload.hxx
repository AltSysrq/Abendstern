/**
 * @file
 * @author Jason Lingle
 * @brief Contains functions for loading images into GL textures
 */

#ifndef IMGLOAD_HXX_
#define IMGLOAD_HXX_

#include <GL/gl.h>
#include <SDL.h>

/** Loads an RGBA image into the given GL texture.
 * @param filename The path to the image to load
 * @param texture A GL texture name to save the image into
 * @return A non-NULL string describing the error
 * on failure.
 */
const char* loadImage(const char* filename, GLuint texture);

/** Loads a greyscale image into the given GL texture.
 * @param filename The path to the image to load
 * @param texture A GL texture name to save the image into
 * @return A non-NULL string describing the error
 * on failure.
 */
const char* loadImageGrey(const char* filename, GLuint texture);

/** If necessary, scale the SDL_Sufrace according to game configuration.
 * This returns either a new SDL_Sufrace or the old one; if a new one
 * is returned, the original has been deleted, unless allowDelete is
 * set to false.
 * The function attempts to preserve aspect ratio, but will not create
 * anything smaller than 4px.
 * This function only works with images from or compatible with those
 * within loadImage; namely, the pixels must be 32-bit in machine
 * byte order.
 *
 * @param surf The input surface
 * @param allowDelete If true, surf will be freed if the return value
 *   is different from surf
 * @param overrideDim If non-zero, use this value for the maximum dimension
 *   instead of conf.graphics.low_res_textures
 * @return The new surface to use; may be the same as surf
 */
SDL_Surface* scaleImage(SDL_Surface* surf, bool allowDelete=true, unsigned overrideDim=0);

#endif /*IMGLOAD_HXX_*/
