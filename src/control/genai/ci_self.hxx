/**
 * @file
 * @author Jason Lingle
 * @brief Contains the cortex_input::Self cortex input provider.
 */

/*
 * ci_self.hxx
 *
 *  Created on: 29.10.2011
 *      Author: jason
 */

#ifndef CI_SELF_HXX_
#define CI_SELF_HXX_

#include <map>
#include <string>
#include <cstring>

#include "src/opto_flags.hxx"

class Ship;

namespace cortex_input {
  /**
   * Used by the Self cortex input provider.
   * Performs actual calculations for that CIP.
   * This should be a base class of GenAI.
   */
  class SelfSource {
    //Despite being private, this still somehow pollutes the namespace
    //of GenAI ("ship is ambiguous: candidates are Controller::ship,
    //SelfSourte::ship, EmotionSource::ship")
    Ship* ssship;
    bool ready;

    protected:
    /**
     * Constructs a SelfSource operating on the given Ship.
     */
    SelfSource(Ship* s) : ssship(s), ready(false) {}

    public:
    /**
     * Defines input offsets for each input provided.
     */
    struct InputOffset { enum t {
      x=0, y, vx, vy, t, vt,
      acc, rota, spin,
      powerumin, powerumax,
      powerprod, capac, rad, mass
    }; };
    /**
     * The number of inputs provided.
     */
    static const unsigned sscip_numInputs = 1+(unsigned)InputOffset::mass;

    private:
    float inputs[sscip_numInputs];

    public:
    /**
     * Copies inputs into the given array.
     * If calculations must still be done, they are performed.
     *
     * Performing these calculations destroys the throttle/accel/brake
     * state of the Ship. Therefore, it is important that reset() be
     * called before any ship-controlling cortex runs.
     */
    void getInputs(float* dst) noth;

    protected:
    /**
     * Resets the source in preparation for the next frame.
     */
    void reset() noth { ready = false; }

    public:
    virtual ~SelfSource() {}
  };

  /**
   * Adds inputs provided by SelfSource to the class tree.
   */
  template<typename Parent>
  class Self: public Parent {
    SelfSource* src; //This is uninitialised until the setSelfSource() call is made

    public:
    static const unsigned cip_first = Parent::cip_last;
    static const unsigned cip_last =    cip_first + SelfSource::sscip_numInputs;
    static const unsigned cip_sx =      cip_first + (unsigned)SelfSource::InputOffset::x;
    static const unsigned cip_sy =      cip_first + (unsigned)SelfSource::InputOffset::y;
    static const unsigned cip_st =      cip_first + (unsigned)SelfSource::InputOffset::t;
    static const unsigned cip_svx =     cip_first + (unsigned)SelfSource::InputOffset::vx;
    static const unsigned cip_svy =     cip_first + (unsigned)SelfSource::InputOffset::vy;
    static const unsigned cip_svt =     cip_first + (unsigned)SelfSource::InputOffset::vt;
    static const unsigned cip_sacc =    cip_first + (unsigned)SelfSource::InputOffset::acc;
    static const unsigned cip_srota =   cip_first + (unsigned)SelfSource::InputOffset::rota;
    static const unsigned cip_sspin =   cip_first + (unsigned)SelfSource::InputOffset::spin;
    static const unsigned cip_spowerumin=cip_first+ (unsigned)SelfSource::InputOffset::powerumin;
    static const unsigned cip_spowerumax=cip_first+ (unsigned)SelfSource::InputOffset::powerumax;
    static const unsigned cip_spowerprod=cip_first+ (unsigned)SelfSource::InputOffset::powerprod;
    static const unsigned cip_scapac =  cip_first + (unsigned)SelfSource::InputOffset::capac;
    static const unsigned cip_srad =    cip_first + (unsigned)SelfSource::InputOffset::rad;
    static const unsigned cip_smass =   cip_first + (unsigned)SelfSource::InputOffset::mass;

    protected:
    Self() {}

    class cip_input_map: public Parent::cip_input_map {
      public:
      cip_input_map() {
        #define I(i) Parent::cip_input_map::ins(#i, cip_##i)
        I(sx);
        I(sy);
        I(st);
        I(svx);
        I(svy);
        I(svt);
        I(sacc);
        I(srota);
        I(sspin);
        I(spowerumin);
        I(spowerumax);
        I(spowerprod);
        I(scapac);
        I(srad);
        I(smass);
        #undef I
      }
    };

    void bindInputs(float* dst) {
      Parent::bindInputs(dst);
      src->getInputs(dst+cip_first);
    }

    /**
     * Sets the SelfSource to use.
     * This MUST be called before the Self will be operational.
     */
    void setSelfSource(SelfSource* s) { src = s; }
  };
}

#endif /* CI_SELF_HXX_ */
