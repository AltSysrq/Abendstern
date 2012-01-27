/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_Idle AI module
 */

/*
 * aim_idle.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_IDLE_HXX_
#define AIM_IDLE_HXX_

#include <libconfig.h++>

#include "src/control/ai/aimod.hxx"

/** core/idle: A trivial AI module that does nothing.
 */
class AIM_Idle: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_Idle(AIControl*,const libconfig::Setting&);
  virtual void action() {}
};

#endif /* AIM_IDLE_HXX_ */
