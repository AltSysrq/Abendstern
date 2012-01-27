/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_EdgeDetect AI module
 */

/*
 * aim_edge_detect.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_EDGE_DETECT_HXX_
#define AIM_EDGE_DETECT_HXX_

#include <libconfig.h++>
#include <string>
#include "src/control/ai/aimod.hxx"

/** state/edge_detect: Switch states if it looks like we're going off the edge.
 *
 * This is based
 * on a number of milliseconds until projections indicate we will
 * cease to exist.
 *
 * @section config Configuration
 * <table>
 *   <tr><td>consider_this_scary</td><td>float</td><td>Milliseconds to project; defaults to 10000.</td></tr>
 *   <tr><td>scared</td><td>string</td><td>State to proceed to in case of fear.</td></tr>
 *   <tr><td>otherwise</td><td>string</td><td>State to go to otherwise; defaults to no change.</td></tr>
 * </table>
 */
class AIM_EdgeDetect: public AIModule {
  float projection;
  std::string scaredName, otherName;

  public:
  /** AIM Standard Constructor */
  AIM_EdgeDetect(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_EDGE_DETECT_HXX_ */
