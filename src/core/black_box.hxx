#ifndef BLACK_BOX_HXX_
#define BLACK_BOX_HXX_

#if defined(DEBUG) || defined(DOXYGEN)
/**
 * Provides a method of recording events such as notable function calls with
 * respect to recent time and nesting.
 *
 * Data is organised into sections; each BlackBox may be present in any number
 * of sections. When a BlackBox is constructed, it formats its message and logs
 * it to each section. Any BlackBox in those sections which is constructed is
 * considered subordinate to this one. On destruction, it is logged that the
 * BlackBox has terminated.
 *
 * If DEBUG is not defined, this class instead becomes a no-op.
 */
class BlackBox {
  const char* sections;

public:
  /**
   * Constructs a BlackBox in the current scope.
   *
   * @param sections The named sections, NUL separated, which this BlackBox
   * records to. The string is terminated by an empty section name (ie, a
   * double NUL).
   * @param format A snprintf-compatible format string.
   * @param ... The arguments to snprintf.
   */
  BlackBox(const char* sections,
           const char* format,
           ...);
  ~BlackBox();

  /**
   * Dumps recent information about the listed sections to stderr.
   *
   * This has the side-effect of emptying the current state.
   */
  static void dump(const char* sections);
};
#else
class BlackBox {
public:
  BlackBox(const char*, ...) {}
  static inline void dump(const char*) {}
};
#endif /* DEBUG or DOXYGEN */

#endif /* BLACK_BOX_HXX_ */
