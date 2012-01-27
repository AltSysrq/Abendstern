/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CellT (CellTarget) cortex input provider.
 */

/*
 * ci_cellt.hxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#ifndef CI_CELLT_HXX_
#define CI_CELLT_HXX_

#include "src/opto_flags.hxx"
#include "src/sim/objdl.hxx"

class Cell;

namespace cortex_input {
  /**
   * Performs actual calculations for the CellT class.
   * The extender of this class must call the reset() function
   * each frame.
   * Since the object has no way to find out when a Cell is
   * destroyed, it maintains the coordinate information itself
   * and performs its own calculations.
   */
  class CellTSource {
    float xoff, yoff, radius, angle;
    ObjDL container;
    bool ready;

    public:
    /** Defines offsets within input arrays for each input. */
    struct InputOffset {
      enum t { cx=0, cy, cvx, cvy };
    };
    static const unsigned ctscip_numInputs = 1+(unsigned)InputOffset::cvy;

    private:
    float inputs[ctscip_numInputs];

    protected:
    CellTSource();

    /**
     * Sets the target Cell for the CellTSource.
     * If it is NULL, reset to zero.
     */
    void setCellTarget(const Cell*) noth;

    /**
     * Copies input data into the given array.
     * If calculations must still be done, they are done now.
     */
    void getInput(float*) noth;

    /**
     * Resets the object so it must recalculate on the next call to getInput().
     */
    void reset() noth { ready = false; }
  };

  /**
   * Provides coordinate and velocity input on a targetted Cell
   * given a CellTSource which performs the actual work.
   * The setCellTSource() function must be called before this class is operational.
   */
  template<typename Parent>
  class CellT: public Parent {
    CellTSource* source;

    protected:
    static const unsigned cip_first = Parent::cip_last;
    static const unsigned cip_last = cip_first + CellTSource::ctscip_numInputs;
    static const unsigned
      cip_cx    = cip_first + (unsigned)CellTSource::InputOffset::cx,
      cip_cy    = cip_first + (unsigned)CellTSource::InputOffset::cy,
      cip_cvx   = cip_first + (unsigned)CellTSource::InputOffset::cvx,
      cip_cvy   = cip_first + (unsigned)CellTSource::InputOffset::cvy;

    class cip_input_map: public Parent::cip_input_map {
      public:
      cip_input_map() {
        #define I(i) Parent::cip_input_map::ins(#i, cip_##i)
        I(cx);
        I(cy);
        I(cvx);
        I(cvy);
        #undef I
      }
    };

    CellT() {}
    void setCellTSource(CellTSource* src) { source = src; }
    void getInput(float* dst) {
      Parent::getInput(dst);
      source->getInput(dst+cip_first);
    }
  };
}

#endif /* CI_CELLT_HXX_ */
