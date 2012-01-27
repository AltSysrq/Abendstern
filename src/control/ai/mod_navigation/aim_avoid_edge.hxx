/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_AvoidEdge AI module
 */

/*
 * aim_avoid_edge.hxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#ifndef AIM_AVOID_EDGE_HXX_
#define AIM_AVOID_EDGE_HXX_

#include <libconfig.h++>
#include "src/control/ai/aimod.hxx"

/** navigation/avoid_edge: Do everything possible to avoid going off an edge.
 *
 * @section gvars Global Variables
 * <table>
 * <tr><td>thrust_angle</td><td>float</td><td>Expected direction of propulsion; defaults to 0.0f.</td></tr>
 * </table>
 */
class AIM_AvoidEdge: public AIModule {
  public:
  /** AIM Standard Constructor */
  AIM_AvoidEdge(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_AVOID_EDGE_HXX_ */
