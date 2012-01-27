#ifndef OPTO_FLAGS_HXX_
#define OPTO_FLAGS_HXX_

/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains definitions used to indicate performance hints to the compiler.
 */

#ifdef DEBUG
  #define FLAT
  #define FAST
  #define HOT
  #define HOTFLAT
  /* When not debugging, noth is defined to throw(), so the compiler
   * can assume that nothing will be thrown and save a lot of time
   * in the function calls.
   */
  #define noth
  #ifdef __GNUC__
    // Used for functions we don't debug but
    // are speed critical
    // This isn't supported for GCC < 4.4 ...
    // And it causes GCC 4.4 to crash. Never mind...
    //#define OPTIMIZE_ANYWAY __attribute__((optimize(3)))
    #define OPTIMIZE_ANYWAY
  #else
    #define OPTIMIZE_ANYWAY
  #endif
#else
  /** Indicates that a function should be fully optimised, despite command-line settings. */
  #define OPTIMIZE_ANYWAY
  /** Indicates a function is never expected to throw any exception.
   * In DEBUG builds, this still lets exceptions be thrown, as it assists with debugging.
   */
  #define noth throw()
  #ifdef __GNUC__
    #ifndef PROFILE
    #define FLAT __attribute__((flatten))
    #endif
    #define FAST __attribute__((fastcall))
    #define HOT __attribute__((hot))
    #define HOTFLAT __attribute__((hot,flatten))
  #else
    /** Indicates that this function's call tree should be flattened. */
    #define FLAT
    /** Indicates to use the FASTCALL calling convention when calling this function. */
    #define FAST
    /** Indicates that this function needs special attention when optimising. */
    #define HOT
    /** Combination of FLAT and HOT. */
    #define HOTFLAT
  #endif
#endif

#endif /*OPTO_FLAGS_HXX_*/
