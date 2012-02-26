/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Contains the AIM_CheckHasWeapons AI module
 */
#ifndef AIM_CHECK_HAS_WEAPONS_HXX_
#define AIM_CHECK_HAS_WEAPONS_HXX_

#include <libconfig.h++>
#include <string>

#include "src/control/ai/aimod.hxx"

/**
 * state/check_has_weapons: Changes state based on whether the ship has weapons
 *
 * @section config Configuration
 * <table>
 *   <tr><td>unarmed</td><td>string</td>
 *       <td>The state to switch to if the ship has no more weapons.</td></tr>
 *   <tr><td>otherwise</td><td>string</td>
 *       <td>The state to switch to otherwise.</td></tr>
 * </table>
 * @section gvars Global Variables
 * <table>
 * <tr>
 *   <td>ship_has_weapons</td>
 *   <td>bool</td>
 *   <td>Whether the ship has weapons; defaults to true</td>
 * </tr>
 * </table>
 */
class AIM_CheckHasWeapons: public AIModule {
  std::string unarmed, otherwise;

  public:
  /** AIM Standard Constructor */
  AIM_CheckHasWeapons(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_CHECK_HAS_WEAPONS_HXX_ */
