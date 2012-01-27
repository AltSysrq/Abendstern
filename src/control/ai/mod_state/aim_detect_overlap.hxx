/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIM_DetectOverlap AI module
 */

/*
 * aim_detect_overlap.hxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#ifndef AIM_DETECT_OVERLAP_HXX_
#define AIM_DETECT_OVERLAP_HXX_

#include <libconfig.h++>
#include <string>
#include "src/control/ai/aimod.hxx"

/** state/detect_overlap: Determines whether we and our target are probably overlapping.
 * Changes state based on the result.
 *
 * @section config Configuration
 * <table>
 *   <tr><td>overlap</td><td>string</td><td>State to change to in case of overlap; defaults to no change.</td></tr>
 *   <tr><td>otherwise</td><td>string</td><td>State to change to otherwise; defaults to no change.</td></tr>
 * </table>
 */
class AIM_DetectOverlap: public AIModule {
  std::string overlap, otherwise;
  public:
  /** AIM Standard Constructor */
  AIM_DetectOverlap(AIControl*, const libconfig::Setting&);
  virtual void action();
};

#endif /* AIM_DETECT_OVERLAP_HXX_ */
