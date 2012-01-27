/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/imgload.hxx
 */

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_image.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "imgload.hxx"
#include "src/globals.hxx"
using namespace std;

static char loadImage_error[4096];

const char* loadImage(const char* filename, GLuint texture) {
  if (headless) return NULL;
  SDL_Surface* source = IMG_Load(filename);
  if (!source) {
    sprintf(loadImage_error, "Error reading image file: %s", filename);
    return loadImage_error;
  }

  //Scale if needed
  source=scaleImage(source);

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
  SDL_Surface* glCompat=SDL_CreateRGBSurface(SDL_SWSURFACE, source->w, source->h, 32, rmask, gmask, bmask, amask);
  SDL_SetAlpha(source, 0, 0);
  SDL_BlitSurface(source, 0, glCompat, 0);
  //Over to OpenGL
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, IMG_SCALE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, IMG_SCALE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glCompat->w, glCompat->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, glCompat->pixels);
  glBindTexture(GL_TEXTURE_2D, 0);

  //Free surfaces
  SDL_FreeSurface(source);
  SDL_FreeSurface(glCompat);

  return NULL;
}

//I hate to copy-and-paste, but I can't see any good way to unify loadImage()
//and loadImageGrey()
const char* loadImageGrey(const char* filename, GLuint texture) {
  if (headless) return NULL;
  SDL_Surface* source = IMG_Load(filename);
  if (!source) {
    sprintf(loadImage_error, "Error reading image file: %s", filename);
    return loadImage_error;
  }

  //Scale if needed
  source=scaleImage(source);

  //Convert to luminance-alpha
  Uint8* greyPx = new Uint8[2 * source->w * source->h];
  Uint32* colourPx = (Uint32*)source->pixels;
  Uint32 rmask = source->format->Rmask, amask = source->format->Amask;
  Uint32 rshiftr=0, ashiftr=0;
  //We'll need to shift the values over to the right an appropriate amount
  while (rmask >> rshiftr != 0xFF) ++rshiftr;
  while (amask >> ashiftr != 0xFF) ++ashiftr;
  for (int y=0; y<source->h; ++y)
  for (int x=0; x<source->w; ++x) {
    //Greyscale, so any colour=luminance, alpha=alpha
    //The surface is in RGBA.
    greyPx[0 + 2*(x+y*source->w)] = (colourPx[x + y*source->w] & rmask) >> rshiftr;
    greyPx[1 + 2*(x+y*source->w)] = (colourPx[x + y*source->w] & amask) >> ashiftr;
  }

  //Over to OpenGL
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, IMG_SCALE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, IMG_SCALE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8_ALPHA8, source->w, source->h,
               0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, greyPx);
  glBindTexture(GL_TEXTURE_2D, 0);

  //Free surfaces
  SDL_FreeSurface(source);
  delete[] greyPx;

  return NULL;
}

SDL_Surface* scaleImage(SDL_Surface* source, bool allowDelete, unsigned overrideDim) {
  int maxdim=(overrideDim? overrideDim : conf["conf"]["graphics"]["low_res_textures"]);
  if (maxdim==0 || (source->w <= maxdim && source->h <= maxdim)) return source;

  //Scaling needed
  int newW, newH;
  if (source->w > source->h) {
    newW=maxdim;
    float aspectH = newW * (source->h/(float)source->w);
    //Find power of two, >=4
    int power=8;
    while (aspectH>=power) power*=2;
    newH=power/2;
  } else {
    newH=maxdim;
    float aspectW = newH * (source->w/(float)source->h);
    int power=8;
    while (aspectW>=power) power*=2;
    newW=power/2;
  }

  //New image
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
  SDL_Surface* scaled=SDL_CreateRGBSurface(SDL_SWSURFACE, newW, newH, 32, rmask, gmask, bmask, amask);

  //Scale (nearest scaling)
  SDL_LockSurface(scaled);
  SDL_LockSurface(source);
  float xscl=source->w/(float)newW;
  float yscl=source->h/(float)newH;
  Uint32* src=(Uint32*)source->pixels, *dst=(Uint32*)scaled->pixels;
  for (int y=0; y<newH; ++y) for (int x=0; x<newW; ++x)
    dst[x+y*newW] = src[ ((int)(x*xscl)) + ((int)(y*yscl))*source->w ];

  SDL_UnlockSurface(source);
  SDL_UnlockSurface(scaled);

  if (allowDelete) SDL_FreeSurface(source);
  return scaled;
}
