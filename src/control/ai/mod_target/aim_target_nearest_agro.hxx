/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_TargetNearest AI module
 */

#ifndef AIM_TARGET_NEAREST_AGRO_HXX_
#define AIM_TARGET_NEAREST_AGRO_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** target/nearest-agro: Select the nearest hostile, powered ship that is
 * observable, but prefer ships with higher playerScores.
 *
 * Set target to NULL if nothing found.
 */
class AIM_TargetNearestAgro: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_TargetNearestAgro(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_TARGET_NEAREST_AGRO_HXX_ */
