/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/audio/synth.hxx
 *
 */

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "synth.hxx"
#include "audio.hxx"
#include "src/globals.hxx"

using namespace std;

namespace audio {
  synthtab sinetab, squaretab, sawtab, ptritab, decaytab, buildtab;

  /**
   * Simple class to call ensureSythtabsGenerated() is called
   * at static-load time, in case nothing else calls it.
   */
  struct MakeTables {
    MakeTables() {
      ensureSythtabsGenerated();
    }
  } static mktables;

  void ensureSythtabsGenerated() {
    static bool hasTables = false;
    if (!hasTables) {
      hasTables = true;
      for (unsigned i=0; i<AUDIO_FRQ; ++i) {
        sinetab  [i] = (Sint16)(sin(i/(float)AUDIO_FRQ*pi*2.0f)*32760.0f);
        squaretab[i] = (i >= AUDIO_FRQ/2? -0x8000 : 0x7FFF);
        sawtab   [i] = (Sint16)(i/(float)AUDIO_FRQ*65536.0f - 32768.0f);
        ptritab  [i] = (Sint16)(32767.0f * (i >= AUDIO_FRQ/2? (AUDIO_FRQ-i)*2/(float)AUDIO_FRQ : i*2/(float)AUDIO_FRQ));
        decaytab [i] = 0x7FFF - (Sint16)(i/(float)AUDIO_FRQ*32767.0f);
        buildtab [i] = (Sint16)(i/(float)AUDIO_FRQ*32767.0f);
      }
    }
  }
};
