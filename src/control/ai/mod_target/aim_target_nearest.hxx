/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_TargetNearest AI module
 */

/*
 * aim_target_nearest.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_TARGET_NEAREST_HXX_
#define AIM_TARGET_NEAREST_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** target/nearest: Select the nearest hostile, powered ship that is
 * observable.
 *
 * Set target to NULL if nothing found.
 */
class AIM_TargetNearest: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_TargetNearest(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_TARGET_NEAREST_HXX_ */
