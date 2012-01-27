/**
 * @file
 * @author Jason Lingle
 * @brief Contains facilities for loading and playing data from raw
 * PCM files.
 */

/*
 * wavload.hxx
 *
 *  Created on: 25.07.2011
 *      Author: jason
 */

#ifndef WAVLOAD_HXX_
#define WAVLOAD_HXX_

//Include GL since MSVC++ has issues with it included after SDL
#include <GL/gl.h>
#include <SDL.h>

#include "audio.hxx"

namespace audio {
  class AudioSource;

  /** Declares the existence of a waveform that is to be loaded.
   *
   * The input file is assumed to be a headerless PCM audio file encoded
   * as 16-bit little-endian; on big-endian architectures, byteswapping
   * is automatically performed. The loader is channels-agnostic.
   *
   * Waveforms are intended to be global-automatic.
   */
  class Waveform {
    friend class WaveformPlayer;

    const char* basename;
    const Sint16* data;
    unsigned datalen;

    public:
    /** Creates a Waveform with the given base filename.
     *
     * @param basename The extensionless part of the filename to use.
     * The actual filename will be something like data/sound/%s.pcm
     * (though that is subject to change).
     */
    Waveform(const char* basename);

    /** Creates and returns an AudioSource that will play the data
     * contained by this Waveform.
     */
    AudioSource* get();

    /** Returns the length of the data. */
    unsigned size();

    private:
    void load();
  };

  /** Simple template to allow using a WaveformPlayer in synthesis. */
  template<Waveform* W>
  class FromWaveform {
    AudioSource*const source;

    public:
    FromWaveform()
    : source(W->get())
    { }

    ~FromWaveform() { delete source; }

    bool get(Sint16* dst, unsigned cnt) {
      return source->get(dst,cnt);
    }
  };
}

#endif /* WAVLOAD_HXX_ */
