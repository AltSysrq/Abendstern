/**
 * @file
 * @author Jason Lingle
 * @brief Contains the cortex_input::Nil dummy cortex input provider.
 */

/*
 * ci_nil.hxx
 *
 *  Created on: 29.10.2011
 *      Author: jason
 */

#ifndef CI_NIL_HXX_
#define CI_NIL_HXX_

#include <map>
#include <string>

/**
 * Contains cortex input providers.
 *
 * Cortex input providers are classes which provide access to certain
 * sets of inputs (since many cortices require the same inputs) and
 * are used as base classes for the various Cortex types.
 *
 * Each input provider must have the following:
 * \verbatim
 *   static const unsigned cip_first    The first (inclusive) input index
 *   static const unsigned cip_last     The last (exclusive) input index
 *   constants for each input
 *   class cip_input_map                Compatible with std::map<std::string,unsigned>
 *   void bindInputs(float*)            Set the inputs to the propper values, given
 *                                      the input array
 * \endverbatim
 *
 * It is the responsibility of each Cortex type to create an instance of
 * the appropriate cip_input_map.
 *
 * The cip_input_map must have a function
 *   void ins(const char*, unsigned)
 */
namespace cortex_input {
  /**
   * cortex_input::Nil is a dummy class to use with the other cortex_input
   * classes. It contains a do-nothing interface required from the
   * template arguments to the CortexInput classes.
   */
  class Nil {
    protected:
    Nil() {}
    static const unsigned cip_first = 0;
    static const unsigned cip_last = 0;
    class cip_input_map: public std::map<std::string,unsigned> {
      protected:
      void ins(const char* s, unsigned i) {
        insert(std::make_pair(std::string(s), i));
      }
    };
    inline void bindInputs(float*) {}
  };
}

#endif /* CI_NIL_HXX_ */
