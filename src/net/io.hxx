/**
 * @file
 * @author Jason Lingle
 * @brief Contains convenience functions for decoding/encoding
 * data independently of endianness.
 */

/*
 * io.hxx
 *
 *  Created on: 08.12.2011
 *      Author: jason
 */

#ifndef IO_HXX_
#define IO_HXX_

#include <cstring>
#include <string>
#include <cassert>

#include <GL/gl.h> //Needed by MSVC++
#include <SDL.h>

typedef Uint8 byte;

namespace io {
  //bcpy copies data in the correct order.
  //On sane architectures, it simply calls memcpy.
  //On others (big-endian), it copies data in reverse order.
  #if SDL_BYTEORDER == SDL_LIL_ENDIAN
    static inline void bcpy(void* dst, const void* src, unsigned len) {
      std::memcpy(dst,src,len);
    }
  #else
    static inline void bcpy(void* dst, const void* src, unsigned len) {
      src += len;
      while (len) {
        *dst++ = *--src;
      }
    }
  #endif

  template<typename D>
  inline void read(const byte*& src, D& dst) {
    bcpy(&dst, src, sizeof(dst));
    src += sizeof(dst);
  }
  template<typename D>
  inline void write(byte*& dst, const D& src) {
    bcpy(dst, &src, sizeof(src));
    dst += sizeof(src);
  }

  static inline void read24(const byte*& src, Uint32 dst) {
    bcpy(&dst, src, 3);
    src += 3;
  }
  static inline void write24(byte*& dst, Uint32 src) {
    bcpy(dst, &src, 3);
    dst += 3;
  }

  static inline void reads(const byte*& src, const byte*const end,
                           std::string& dst) {
    unsigned len=0;
    for (const byte* i=src; i < end && *i; ++i) ++len;
    dst.assign(src, src+len);
    src += len;
    //Pass over terminating NUL if there was one
    if (src != end) ++src;
  }
  static inline void writes(byte*& dst, byte*const end,
                            const std::string& src) {
    assert(((unsigned)(end-dst)) > src.size());
    memcpy(dst, src.c_str(), src.size()+1);
    dst += src.size()+1;
  }
}

#endif /* IO_HXX_ */
