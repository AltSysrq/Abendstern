/**
 * @file
 * @author Jason Lingle
 * @brief Contains the CellCreationBrush mode
 */

/*
 * cell_creation_brush.hxx
 *
 *  Created on: 07.02.2011
 *      Author: jason
 */

#ifndef CELL_CREATION_BRUSH_HXX_
#define CELL_CREATION_BRUSH_HXX_

#include <string>
#include <vector>

#include "manipulator_mode.hxx"

class Cell;

/** The CellCreationBrush allows the user to paint new cells
 * onto the ship. This is done with a two-state system, described
 * below. An "event" below refers to any motion event when the
 * primary button is pressed.
 * + Waiting state. When an event occurs, the cell the event was
 *   triggered on is considered the active cell. A new cell of
 *   the desired type is spawned at each free side, the ship
 *   reloaded. If any spawning fails, that cell is removed. In
 *   the case of right triangles, all three possible orientations
 *   are tried before giving up. The state is changed to active.
 * + Active state. Events on the active cell are ignored. Events
 *   on one of the temporary cells cause the other temporary cells
 *   to be deleted and the state is set to waiting. An event on
 *   any other cell first discards all temporary cells. Then, the
 *   event cell is made active and new temporaries are spawned, as
 *   if transferring from waiting to active.
 *
 * Releasing the mouse will delete any temporaries and return to
 * waiting state.
 *
 * When the mouse button is pushed, the undo state is pushed
 * as well. If no changes have been made when the button is
 * released, the undo state is dropped.
 *
 * If the brush is deactivated in active mode, all temporaries
 * are removed.
 */
class CellCreationBrush: public ManipulatorMode {
  /* Names of temporary cells. An empty string indicates
   * that there is no temporary in that slot. This array
   * corresponds exactly with that of the active cell's
   * neighbour array.
   */
  std::string temporaries[4];

  /* Set to false when the mode is activated, and to
   * true when any permanent modifications have been
   * made.
   */
  bool modificationsDuringActivation;

  /* If true, we are currently active. */
  bool isActive;

  /* Contains the name of the currently active cell */
  std::string activeCell;

  /* Keeps track of cells created by the current paint
   * stroke. We need to draw these ourselves; having the
   * Ship reset its graphics continuously takes too much
   * time. Also, this way we can indicate true and potential
   * cells.
   */
  std::vector<Cell*> paintCells;
  /* If we reject a new cell due to collision, store it
   * here to show them why.
   */
  std::vector<Cell*> whyNotCells;

  public:
  /** Standard mode constructor */
  CellCreationBrush(Manipulator*, Ship*const&);

  virtual void activate();
  virtual void deactivate();
  virtual void motion(float,float,float,float);
  virtual void press(float,float);
  virtual void release(float,float);
  virtual void draw();

  private:
  /* Performs transition into active state. This has several
   * effects:
   * + The undo state is pushed
   * + Temporaries are spawned
   * + The ship is reloaded multiple times
   * + The active state is set appropriately
   * The function returns true if there were any temporaries
   * spawned. If it returns false, it is not possible to
   * add any new cells of the given type here, and the
   * undo state has been dropped as well as the active
   * state being set to false again.
   */
  bool enterActiveMode(Cell*);

  /* Performs the transition out of active state. This
   * has the following effects:
   * + If the specified cell is not one of the temporaries,
   *   the undo state is poped; otherwise, it is comitted
   *   after deleting the temporaries other than the event
   *   cell
   * + The ship is reloaded
   * + The active state is set to false
   *
   * The cell passed may be NULL. If the brush is not currently
   * in the active state, this function does nothing.
   */
  void exitActiveMode(Cell*);

  /* Attempts to spawn a temporary of the current type
   * at the given index, which may be greater than the
   * number of neighbours the cell supports. Returns
   * true if the spawning succeeded, false otherwise.
   * This also generates the name for the cell and stores
   * it in the temporaries array.
   * Affects the active cell. The currently-loaded ship
   * is modified, and the ship will be reloaded at least
   * once.
   */
  bool spawnTemporary(unsigned);

  /* Generate a name for a new cell and store it in
   * the given string. The new name is guaranteed
   * to be unique and a valid libconfig identifier.
   */
  void generateName(std::string&);

  /* Searches for other cells that should be bound as
   * neighbours to the given cell. This function assumes
   * that the cell is properly present and oriented within
   * the current ship. It adds necessary entries to the
   * config file, but does NOT modify the current ship.
   *
   * Returns false if it detects cell overlap.
   */
  bool detectInterconnections(Cell*);
};

#endif /* CELL_CREATION_BRUSH_HXX_ */
