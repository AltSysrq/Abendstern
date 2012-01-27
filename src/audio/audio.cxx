/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/audio/audio.hxx.
 */

#include <vector>
#include <cstring>
#include <iostream>

#include <SDL.h>

#include "audio.hxx"
#include "synth.hxx"

using namespace std;

namespace audio {
  Mixer trueRoot;
  VMixer& root(*new VMixer(0x1000, 0x3000));

  static void callback(void*, Uint8*, int);

  static SDL_AudioSpec spec = {
    AUDIO_FRQ,
    #if SDL_BYTEORDER == SDL_LIL_ENDIAN
    AUDIO_S16LSB,
    #else
    AUDIO_S16MSB,
    #endif /* endianness */
    2, //channels
    0, //silence
    0, //padding -- WTH is this not documented?
    1024, //samples (buffer size, in samples)
    1024*4, //size (buffer, in bytes)
    callback,
    NULL
  };

  Mixer::Mixer(unsigned nc, Sint16 bvm)
  : nominalCount(nc), volume(bvm)
  { }

  Mixer::~Mixer() {
    for (unsigned i=0; i<sources.size(); ++i)
      delete sources[i];
  }

  bool Mixer::get(Sint16* dst, unsigned cnt) {
    while (cnt > lenof(tmp)) {
      //Special case, we need to break this into parts
      //since our temporary buffer isn't big enough

      //Explicitly call this same function
      Mixer::get(dst, lenof(tmp));
      dst += lenof(tmp);
      cnt -= lenof(tmp);
    }

    if (sources.empty()) {
      //Special case, silence
      memset(dst, 0, cnt*2);
    } else if (sources.size() == 1) {
      //Handle this much simpler case specially
      //We can just copy the data over, then multiply by volume
      bool keep = sources[0]->get(dst, cnt);
      if (!keep) sources.clear();
      for (unsigned j=0; j<cnt; ++j)
        muleq(dst[j], volume);
    } else {
      Sint16 volmul;
      if (sources.size() > nominalCount)
        volmul = mul(volume, frac(nominalCount, sources.size()));
      else
        volmul = volume;
      //Begin by zeroing the buffer
      memset(dst, 0, cnt*2);
      //For each source...
      for (unsigned i=0; i<sources.size(); ++i) {
        //Get its data; remove source after if it ceases to exist
        if (!sources[i]->get(tmp, cnt))
          sources.erase(sources.begin() + i--);
        //Add all values from the source, times the volume multiplier,
        //to the destination buffer
        //Extra check to handle clipping
        for (unsigned j=0; j<cnt; ++j) {
          Sint16 d = mul(tmp[j], volmul);
          signed c = dst[j];
          c += d;
          if (c > 0x7FFF) c = 0x7FFF;
          else if (c < -0x8000) c = -0x8000;
          dst[j] = c;
        }
      }
    }

    return true;
  }

  void Mixer::add(AudioSource* src) {
    SDL_LockAudio();
    sources.push_back(src);
    SDL_UnlockAudio();
  }

  static bool wasInit = false;
  void init() {
    if (wasInit) return;

    if (SDL_OpenAudio(&spec, NULL /* automatically convert */)) {
      cerr << "WARN: Unable to open audio device: " << SDL_GetError() << endl;
      return;
    }
    if (!trueRoot.size())
      trueRoot.add(&root);
    SDL_PauseAudio(0);
    wasInit = true;

    //For testing
//    root.add(new Virtual<Duplex<
//             > >);
  }

  void term() {
    if (!wasInit) return;

    SDL_PauseAudio(1);
    SDL_CloseAudio();
  }

  static void callback(void*, Uint8* dat, int length) {
    //Split the buffer for better accuracy
    Sint16* dst = (Sint16*)dat;
    unsigned len = length/2;
    while (len > 0) {
      unsigned l = (len > 1024? 1024 : len);
      trueRoot.get(dst, l);
      dst += l;
      len -= l;
    }
  }
}
