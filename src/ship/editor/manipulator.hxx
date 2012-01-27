/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Manipulator class.
 * @see src/ship/editor/editor_design.txt
 */

/*
 * manipulator.hxx
 *
 *  Created on: 05.02.2011
 *      Author: jason
 *
 * See editor_design.txt for information on the overall design of
 * the new ship editor.
 */

#ifndef MANIPULATOR_HXX_
#define MANIPULATOR_HXX_

#include <map>
#include <deque>
#include <string>
#include <libconfig.h++>

#include "src/core/aobject.hxx"
#include "src/graphics/mat.hxx"

class ManipulatorMode;
class Ship;
class GameField;
class Cell;

/** The core manipulator class. It assumes that the scratch
 * intercommunication config has been mounted to "edit".
 */
class Manipulator: public AObject {
  std::map<std::string,ManipulatorMode*> modes;
  std::deque<libconfig::Config*> undoStates, historyStates;

  ManipulatorMode* currentMode;
  Ship* ship;
  GameField* field;

  matrix transform;

  /* Every time a non-left button is pressed, we increment this;
   * we decrement it every time one is released. Motion events
   * when this is non-zero alter the current pan.
   */
  int panButtonDownCount;

  public:
  /** Initialises the Manipulator */
  Manipulator();
  virtual ~Manipulator();

  /** Perform any updates and actions necessary. This should be
   * called every frame.
   */
  void update();

  /** Draw the ship and any auxilliary information. */
  void draw();

  /** Handle a left mouse down at the specified screen ([0,0]x[1,vheight]) coords. */
  void primaryDown(float,float);

  /** Handle a left mouse up at the specified screen coords. */
  void primaryUp(float,float);

  /** Handle a non-left mouse button down at the specified screen coordinates. */
  void secondaryDown(float,float);

  /** Handle a non-left mouse button up at the specified screen coordinates. */
  void secondaryUp(float,float);

  /** Handle a scroll-up event. */
  void scrollUp(float,float);

  /** Handle a scroll-down event. */
  void scrollDown(float,float);

  /** Handle mouse motion. The first two arguments are the NEW screen coordinates,
   * while the second two are the relative motion in screen coordinates.
   */
  void motion(float x,float y,float mx,float my);

  /** Resets the view to the identity matrix */
  void resetView();

  /** Copies the current Ship description into a new undo state. */
  void pushUndo();

  /** Restores the most recent undo state. */
  void popUndo();

  /** Forgets the most recent undo state, effectively making
   * that change permanent with respect to the stack.
   */
  void commitUndo();

  /**
   * Deactivates the current mode, even if it should still be considered active.
   * This should be done when ship modifications outside of the mode are to be
   * made.
   */
  void deactivateMode();

  /**
   * Reactivates the current mode.
   * @see deactivateMode()
   */
  void activateMode();

  /** Indicates that the current state is consistent and could be
   * a state the user might want to revert to.
   * This permanently adds a copy of the current state to the
   * history list.
   */
  void addToHistory();

  /** Reverts the current state to the history entry at the given
   * index.
   */
  void revertToHistory(unsigned);

  /** Reload the current ship. If successful, return NULL.
   * Otherwise, return error message and keep the current
   * Ship.
   *
   * If this fails, it indicates that the current configuration
   * is not valid.
   */
  const char* reloadShip();

  /** Destroys the current ship. */
  void deleteShip();

  /** Copies data from the source to the destination, both strings
   * representing mount roots.
   */
  void copyMounts(const char* dst, const char* src);

  /** Returns the Cell associated with the given screen coordinate, or
   * NULL if it points to nothing.
   */
  Cell* getCellAt(float,float);

  /** Converts screen coordinates to field coordinates. */
  void screenToField(float&,float&);
};

#endif /* MANIPULATOR_HXX_ */
