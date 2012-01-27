/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CellEx cortex input provider.
 */

/*
 * ci_cellex.hxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#ifndef CI_CELLEX_HXX_
#define CI_CELLEX_HXX_

#include "src/opto_flags.hxx"

class Cell;

namespace cortex_input {
  /** Used internally by the CellEx class. */
  struct CellExInputOffset {
    enum t { celldamage=0, cxo, cyo, cnsidesfree, cbridge, sysclass };
  };
  /**
   * Used internally by the CellEx class.
   */
  void cellExExamineCell(const Cell*, float* dst);
  /**
   * The CellEx class provides inputs for examining a Cell
   * (not for tracking a targetted Cell). While it does create
   * a slot for sysclass, it does not actually set the value
   * for that input, since doing that is accomplished by
   * work that must be done anyway to determine which output
   * must be used.
   *
   * Note that this class's getInputs() function does NOT set
   * the inputs for this class! Use getCellExInputs(const Cell*,float*)
   * for this purpose.
   */
  template<typename Parent>
  class CellEx: public Parent {
    protected:
    CellEx() {}

    static const unsigned cip_first = Parent::cip_last;
    static const unsigned cip_last = cip_first + 1 + (unsigned)CellExInputOffset::sysclass;
    static const unsigned
      cip_cxo           = cip_first + (unsigned)CellExInputOffset::cxo,
      cip_cyo           = cip_first + (unsigned)CellExInputOffset::cyo,
      cip_celldamage    = cip_first + (unsigned)CellExInputOffset::celldamage,
      cip_cnsidesfree   = cip_first + (unsigned)CellExInputOffset::cnsidesfree,
      cip_cbridge       = cip_first + (unsigned)CellExInputOffset::cbridge,
      cip_sysclass      = cip_first + (unsigned)CellExInputOffset::sysclass;

    class cip_input_map: public Parent::cip_input_map {
      public:
      cip_input_map() {
        #define I(i) Parent::cip_input_map::ins(#i, cip_##i)
        I(cxo);
        I(cyo);
        I(celldamage);
        I(cnsidesfree);
        I(cbridge);
        I(sysclass);
        #undef I
      }
    };

    /**
     * The getInputs() function does not set the values for a
     * particular Cell.
     * @see getCellExInputs(const Cell*,float*)
     */
    void getInputs(float* dst) noth { Parent::getInputs(dst); }
    /**
     * Sets the inputs for the given Cell.
     */
    void getCellExInputs(const Cell* cell, float* dst) const noth {
      cellExExamineCell(cell, dst+cip_first);
    }
  };
}

#endif /* CI_CELLEX_HXX_ */
