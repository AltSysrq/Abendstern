/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AbstractSystemPainter mode
 */

/*
 * abstract_system_painter.hxx
 *
 *  Created on: 08.02.2011
 *      Author: jason
 */

#ifndef ABSTRACT_SYSTEM_PAINTER_HXX_
#define ABSTRACT_SYSTEM_PAINTER_HXX_

#include "manipulator_mode.hxx"

class Cell;

/** The AbstractSystemPainter is an abstract ManipulatorMode used as a base
 * class by the SystemCreationBrush and SystemCreationRect classes.
 *
 * It handles actual creation of systems, as well as calling
 * release() on deactivation, since the ship editor may switch
 * modes during painting.
 */
class AbstractSystemPainter: public ManipulatorMode {
  protected:
  /** Standard mode constructor */
  AbstractSystemPainter(Manipulator*,Ship*const&);

  /** Create systems in the specified cell. Real systems
   * are created in the current ship, as well as entries
   * in the config file IF the systems accept the cell.
   * If either system is rejected, an error message is
   * returned (if both are, which one is not defined, but
   * it will be one of them). Otherwise, returns NULL.
   */
  const char* paintSystems(Cell*);

  public:
  virtual void deactivate();
};

#endif /* ABSTRACT_SYSTEM_PAINTER_HXX_ */
