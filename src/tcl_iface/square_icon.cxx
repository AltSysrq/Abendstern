/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/tcl_iface/square_icon.hxx
 */

/*
 * square_icon.cxx
 *
 *  Created on: 11.09.2011
 *      Author: jason
 */

#include <cstring>
#include <iostream>

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_image.h>
#include <png++/png.hpp>

#include "square_icon.hxx"
#include "src/graphics/square.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/imgload.hxx" //For scaleImage
#include "src/globals.hxx"

using namespace std;
using namespace png;

SquareIcon::SquareIcon()
: tex(0), surf(NULL)
{ }

SquareIcon::~SquareIcon() {
  unload();
}

bool SquareIcon::load(const char* filename, unsigned size, SquareIcon::LoadReq req) {
  unload();
  if (headless) return false;

  surf = IMG_Load(filename);
  if (!surf) return false;

  bool needsScaling = false;

  switch (req) {
    case Strict:
      if (surf->w != (int)size || surf->h != (int)size) {
        unload();
        return false;
      }
      break;
    case Lax:
      if (surf->w > (int)size || surf->h > (int)size) {
        unload();
        return false;
      }
      break;
    case Scale:
      if (surf->w > (int)size || surf->h > (int)size)
        needsScaling = true;
      break;
  }

  //Convert to OpenGL-compatible
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 rmask=0xff000000,
           gmask=0x00ff0000,
           bmask=0x0000ff00,
           amask=0x000000ff;
  #else
    Uint32 rmask=0x000000ff,
           gmask=0x0000ff00,
           bmask=0x00ff0000,
           amask=0xff000000;
  #endif

  //GL-compatible surface
  SDL_Surface* glCompat=SDL_CreateRGBSurface(SDL_SWSURFACE, surf->w, surf->h, 32, rmask, gmask, bmask, amask);
  SDL_SetAlpha(surf, 0, 0);
  SDL_BlitSurface(surf, 0, glCompat, 0);

  SDL_FreeSurface(surf);
  surf = glCompat;

  if (needsScaling)
    surf = scaleImage(surf, true, size);

  unsigned powerw = 1, powerh = 1;
  while (powerw < (unsigned)surf->w) powerw <<= 1;
  while (powerh < (unsigned)surf->h) powerh <<= 1;
  if (((unsigned)surf->w) != powerw || ((unsigned)surf->h) != powerh) {
    //Need to reblit to power-of-two
    glCompat = SDL_CreateRGBSurface(SDL_SWSURFACE, powerw, powerh, 32, rmask, gmask, bmask, amask);
    SDL_SetAlpha(surf, 0, 0);
    SDL_BlitSurface(surf, 0, glCompat, 0);
    SDL_FreeSurface(surf);
    surf = glCompat;
  }

  //Over to OpenGL
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, IMG_SCALE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, IMG_SCALE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
  glBindTexture(GL_TEXTURE_2D, 0);
  return true;
}

void SquareIcon::unload() {
  if (tex) {
    glDeleteTextures(1, &tex);
    tex = 0;
  }
  if (surf) {
    SDL_FreeSurface(surf);
    surf = NULL;
  }
}

bool SquareIcon::isLoaded() const {
  return tex != 0;
}

void SquareIcon::draw() const {
  if (!isLoaded()) return;

  glBindTexture(GL_TEXTURE_2D, tex);
  square_graphic::bind();
  shader::textureReplaceU uni = { 0 };
  shader::textureReplace->activate(&uni);
  square_graphic::draw();
}

bool SquareIcon::save(const char* filename) const {
  if (!surf) return false;
  png::image<rgba_pixel> img(surf->w, surf->h);
  const Uint32* dat = (const Uint32*)surf->pixels;
  const SDL_PixelFormat& fmt(*surf->format);
  for (unsigned y=0; y<(unsigned)surf->h; ++y) {
    for (unsigned x=0; x<(unsigned)surf->w; ++x) {
      Uint32 p = dat[x + y*surf->w];
      img[y][x] = rgba_pixel((p & fmt.Rmask) >> fmt.Rshift,
                             (p & fmt.Gmask) >> fmt.Gshift,
                             (p & fmt.Bmask) >> fmt.Bshift,
                             (p & fmt.Amask) >> fmt.Ashift);
    }
  }

  //write cannot return failure.
  //Does it throw an exception? The documentation does not indicate.
  //Assume it does.
  try {
    img.write(filename);
  } catch (...) {
    return false;
  }
  return true;
}
