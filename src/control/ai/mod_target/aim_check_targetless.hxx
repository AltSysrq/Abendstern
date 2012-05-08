/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_CheckTargetless AI module
 */

#ifndef AIM_CHECK_TARGETLESS_HXX_
#define AIM_CHECK_TARGETLESS_HXX_

#include <string>
#include <libconfig.h++>

#include "src/control/ai/aimod.hxx"

/**
 * Switches state based on whether a living target exists.
 * @section config Configuration
 * <table>
 *   <tr><td>targetless</td><td>string</td>
 *       <td>The state to switch to if the ship has no living target.</td></tr>
 *   <tr><td>otherwise</td><td>string</td>
 *       <td>The state to switch to otherwise.</td></tr>
 * </table>
 */
class AIM_CheckTargetless: public AIModule {
  std::string targetless, otherwise;

  public:
  /** AIM Standard Constructor */
  AIM_CheckTargetless(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_CHECK_TARGETLESS_HXX_ */
