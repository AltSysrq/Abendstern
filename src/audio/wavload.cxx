/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/audio/wavload.hxx
 */

/*
 * wavload.cxx
 *
 *  Created on: 25.07.2011
 *      Author: jason
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <vector>

#include "wavload.hxx"
#include "audio.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

namespace audio {
  class WaveformPlayer: public AudioSource {
    const Waveform*const that;
    unsigned ix;

    public:
    WaveformPlayer(const Waveform* t)
    : that(t), ix(0)
    { }

    virtual bool get(Sint16* dst, unsigned cnt) {
      unsigned l = (that->datalen - ix > cnt? cnt : that->datalen - ix);
      memcpy(dst, that->data+ix, l*2);
      ix += l;
      memset(dst+l, 0, 2*(cnt-l));
      return ix < that->datalen;
    }
  };

  Waveform::Waveform(const char* bn)
  : basename(bn), data(NULL)
  { }

  AudioSource* Waveform::get() {
    if (!data) load();
    return new WaveformPlayer(this);
  }

  unsigned Waveform::size() {
    if (!data) load();
    return datalen;
  }

  void Waveform::load() {
    static char filename[256];
    Waveform* w = this;

    sprintf(filename, "data/sound/%s.pcm", w->basename);

    //Using the C IO library, since it actually allows us to find the length
    //of a file instantly
    FILE* f = fopen(filename, "rb");
    if (!f) {
      printf("FATAL: Unable to open %s: %s\n", filename, strerror(errno));
      exit(EXIT_MALFORMED_DATA);
    }

    //Find the length of the file
    if (fseek(f, 0, SEEK_END)) {
      printf("FATAL: Error seeking to end of %s: %s\n", filename, strerror(errno));
      exit(EXIT_PLATFORM_ERROR);
    }
    signed long length = ftell(f);
    if (length < 0) {
      printf("FATAL: Error getting length of %s: %s\n", filename, strerror(errno));
      exit(EXIT_PLATFORM_ERROR);
    }
    if (length & 1) {
      printf("FATAL: Odd length of %s\n", filename);
      exit(EXIT_MALFORMED_DATA);
    }
    //Back to beginning
    if (fseek(f, 0, SEEK_SET)) {
      printf("FATAL: Error rewinding %s: %s\n", filename, strerror(errno));
      exit(EXIT_PLATFORM_ERROR);
    }

    //Allocate data
    Sint16* data = new Sint16[length/2];
    w->data = data;
    w->datalen = (unsigned)length/2;
    //Read it in
    if (length != (signed long)fread(data, 1, length, f)) {
      printf("FATAL: Could not fully read %s: %s\n", filename, strerror(errno));
      exit(EXIT_PLATFORM_ERROR);
    }
    //Perform byteswapping if needed
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
      for (unsigned i=0; i<length/2; ++i) {
        Sint16 d = data[i];
        data[i] = SDL_Swap16(d);
      }
    #endif

    //Done with file
    fclose(f);
  }
}
