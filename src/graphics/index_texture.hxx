/**
 * @file
 * @author Jason Lingle
 * @brief Provides utilities for loading RGBA textures and converting
 * them for use in index-based shaders.
 *
 * Information on conversion is read from a libconfig file. Each
 * top-level entry has the name of the class the texture corresponds
 * to. A sub-key named "src" provides the string base filename of
 * the PNG image for the texture (ie, images/sys/capacitr.png would
 * be indicated by "capacitr"). The colourmap is another sub-key
 * named "colourmap", which is an array of integers whose length
 * must be evenly divisible by 3. The values cycle through the
 * following meanings:
 * \verbatim
 *   0  31-bit ARGB minimum, inclusive
 *   1  31-bit ARGB maximum, inclusive
 *   2  8-bit index
 * \endverbatim
 *
 * The two ARGB values are treated as 31-bit, since it may not be
 * possible to write 32-bit safe code in all languages. The components
 * are separated as follows, after ensuring that bit 31 is 0:
 * \verbatim
 *   red = (argb >> 16) & 0xFF
 *   grn = (argb >> 8 ) & 0xFF
 *   blu = argb & 0xFF
 *   alp = (argb >> 23) & (0xFE | ((argb >> 24) & 1))
 * \endverbatim
 * The alpha derivation is a bit odd. In words: We only have 7 bits to
 * work with, but we need to get 8 bits out. Additionally, 0x00 must map
 * to 0x00 and 0x7F to 0xFF. Therefore, we essentially shift the would-be
 * result one to the left, then set bit 0 to the value of bit 1.
 *
 * Each tripple in the colourmap defines a single range mapping. Each pixel
 * in the source RGBA image is disassembled into its individual components.
 * If ALL components are each greater than or equal to the corresponding
 * component of the ARGB minimum, and less than or equal to the corresponding
 * component of the ARGB maximum, that pixel matches that mapping, and is
 * substituted with the specified index. If no range matches, an error
 * is raised.
 */

/*
 * index_texture.hxx
 *
 *  Created on: 26.01.2011
 *      Author: jason
 */

#ifndef INDEX_TEXTURE_HXX_
#define INDEX_TEXTURE_HXX_

#include <libconfig.h++>
#include <SDL.h>
#include <GL/gl.h>

/** Reads in and processes all textures specified, from the given libconfig file.
 * @param conf The filename of the libconfig file to source.
 * @param prefix The path to place to the left of the filename.
 * The remainder of the arguments take the form
 * \verbatim
 *   const char*        Class name
 *   GLuint*            Destination in which to store texture.
 *                      Each texture is stored as greyscale.
 * \verbatim
 * The arguments list must be terminated with a NULL.
 *
 * If any error occurs, a message is printed to the terminal
 * and the program is aborted.
 */
void loadIndexTextures(const char* conf, const char* prefix, ...);

/** Converts a single texture and stores it in a GLuint, which it
 * returns. As with loadIndexTextures, the texture is greyscale.
 * @param Conf A const libconfig::Setting& which is the actual colour-map to use.
 * @param tex The raw image data, assumed to be in standard OpenGL format
 * (RGBA from a big-endian persepective).
 * @param w Width of the texture. This should be evenly divisible by 8,
 *   since the function does not tell OpenGL what the alignment is
 *   (and therefore it could be anything up to 8 bytes).
 * @param h The height of the texture.
 */
GLuint indexTexture(const libconfig::Setting& conf, const Uint32* tex, unsigned w, unsigned h);

/** Converts a single texture, writing it into the given destination.
 * @param conf A const libconfig::Setting& which is the actual colour-map to use.
 * @param tex The raw image data, assumed to be in standard OpenGL format
 * (RGBA from a big-endian perspective).
 * @param len The number of pixels.
 * @param dst The destination buffer, an array of bytes at least len long.
 */
void convertImageToIndex(const libconfig::Setting& conf, const Uint32* tex, unsigned len, unsigned char* dst);

#endif /* INDEX_TEXTURE_HXX_ */
