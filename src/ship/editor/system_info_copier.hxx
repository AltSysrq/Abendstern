/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SystemInfoCopier mode
 */

/*
 * system_info_copier.hxx
 *
 *  Created on: 09.02.2011
 *      Author: jason
 */

#ifndef SYSTEM_INFO_COPIER_HXX_
#define SYSTEM_INFO_COPIER_HXX_

#include "manipulator_mode.hxx"

/**
 * Mode which allows the user to copy system information.
 *
 * The SystemInfoCopier waits until the user clicks on a cell.
 * When that happens, it copies that cell's system information
 * into the new system information. To be safe, it copies
 * the lack of a system as "No change". Immediately after the
 * copy, it sets the editor mode to "none" to signal completion.
 */
class SystemInfoCopier: public ManipulatorMode {
  public:
  /** Standard mode constructor. */
  SystemInfoCopier(Manipulator*,Ship*const&);
  virtual void activate();
  virtual void press(float,float);
};

#endif /* SYSTEM_INFO_COPIER_HXX_ */
