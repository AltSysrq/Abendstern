/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/index_texture.hxx
 */

/*
 * index_texture.cxx
 *
 *  Created on: 26.01.2011
 *      Author: jason
 */

#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <string>

#include <GL/gl.h>
#include <libconfig.h++>
#include <SDL.h>
#include <SDL_image.h>

#include "index_texture.hxx"
#include "src/exit_conditions.hxx"

using namespace libconfig;
using namespace std;

struct maprng {
  unsigned char a1,r1,g1,b1,a2,r2,g2,b2,index;
};

void convertImageToIndex(const Setting& conf, const Uint32* data, unsigned len, unsigned char* dst) {
  //First, load the map
  vector<maprng> map;
  try {
    for (unsigned i=0; i<(unsigned)conf.getLength(); i+=3) {
      unsigned min=conf[i], max=conf[i+1];
      unsigned char ix=(unsigned)conf[i+2];

      min &= 0x7FFFFFFF;
      max &= 0x7FFFFFFF;

      maprng m = {
        ((min >> 23) & 0xFE) | ((min >> 24)&1),
        (min >> 16) & 0xFF,
        (min >> 8) & 0xFF,
        min & 0xFF,
        ((max >> 23) & 0xFE) | ((max >> 24)&1),
        (max >> 16) & 0xFF,
        (max >> 8) & 0xFF,
        max & 0xFF,
        ix
      };
      map.push_back(m);

//      printf("Map[%d]: R:(%d--%d) G:(%d--%d) B:(%d--%d) A:(%d--%d)\n",
//             i,
//             (int)m.r1, (int)m.r2, (int)m.g1, (int)m.g2, (int)m.b1, (int)m.b2, (int)m.a1, (int)m.a2);
    }
  } catch (...) {
    cerr << "FATAL: Error while parsing colourmap!" << endl;
    exit(EXIT_MALFORMED_CONFIG);
  }

  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const unsigned rshift=24, gshift=16, bshift=8, ashift=0;
  #else
    const unsigned rshift=0, gshift=8, bshift=16, ashift=24;
  #endif

  //Now perform conversion
  for (; len; ++data, ++dst, --len) {
    unsigned r = (*data >> rshift) & 0xFF,
             g = (*data >> gshift) & 0xFF,
             b = (*data >> bshift) & 0xFF,
             a = (*data >> ashift) & 0xFF;
    for (unsigned i=0; i<map.size(); ++i) {
      if (r >= map[i].r1
      &&  g >= map[i].g1
      &&  b >= map[i].b1
      &&  a >= map[i].a1
      &&  r <= map[i].r2
      &&  g <= map[i].g2
      &&  b <= map[i].b2
      &&  a <= map[i].a2) {
        *dst = map[i].index;
        goto next_pixel;
      }
    }

    //Couldn't find
    cerr << "FATAL: Could not find a match for (" << r << ',' << g << ',' << b << ',' << a
         << ") in the colourmap!" << endl;
    exit(EXIT_MALFORMED_DATA);

    next_pixel:;
  }
}

GLuint indexTexture(const Setting& conf, const Uint32* data, unsigned w, unsigned h) {
#ifndef AB_OPENGL_14
  unsigned char* dst = new unsigned char[w*h];
  //Any failure kills the program, so we don't have to see if we must
  //deallocate dst
  convertImageToIndex(conf, data, w*h, dst);
#else
  const unsigned char* dst = (const unsigned char*)data;
#endif

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  //Anything other than NEAREST is meaningless for indexed images
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#ifndef AB_OPENGL_14
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, dst);
  delete[] dst;
#else
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst);
#endif

  return tex;
}

void loadIndexTextures(const char* conffile, const char* prefix, ...) {
  Config conf;
  try {
    conf.readFile(conffile);
  } catch (...) {
    cerr << "FATAL: Unable to load colourmap conf from " << conffile << endl;
    exit(EXIT_MALFORMED_CONFIG);
  }

  Setting& root(conf.getRoot());

  va_list args;
  va_start(args, prefix);

  const char* className;
  GLuint* tex;
  for (className=va_arg(args,const char*); className;
       className=va_arg(args,const char*)) {
    tex=va_arg(args, GLuint*);
    string filename(prefix);
    //cout << className << endl;
    try {
      Setting& clazz(root[className]);
      filename += (const char*)clazz["src"];
    } catch (...) {
      cerr << "FATAL: Colourmap class " << className << " missing or malformed" << endl;
      exit(EXIT_MALFORMED_CONFIG);
    }

    filename += ".png";

    SDL_Surface* source = IMG_Load(filename.c_str());
    if (!source) {
      cerr << "Error reading image file: " << filename << endl;
      exit(EXIT_MALFORMED_DATA);
    }

    //Convert to ensure compatibility with OpenGL
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

    //Convert to indexed mode and bind
    Setting* colourmap;
    try {
      colourmap = &root[className]["colourmap"];
    } catch (...) {
      cerr << "FATAL: colourmap key missing from class " << className << endl;
      exit(EXIT_MALFORMED_CONFIG);
    }
    *tex = indexTexture(*colourmap, (Uint32*)glCompat->pixels, glCompat->w, glCompat->h);

    //Free surfaces
    SDL_FreeSurface(glCompat);
    SDL_FreeSurface(source);
  }
}

