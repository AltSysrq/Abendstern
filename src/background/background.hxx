/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the abstract Background class.
 * @see src/background/star_field.hxx
 * @see src/background/planet.hxx
 * @see src/background/nebula.hxx
 */

#ifndef BACKGROUND_HXX_
#define BACKGROUND_HXX_

#include "src/camera/effects_handler.hxx"
#include "src/opto_flags.hxx"

class GameField;
class GameObject;

/** This is the abstract class for all types of Backgrounds.
 * @see StarField
 * @see Planet
 * @see Nebula
 */
class Background: public EffectsHandler {
  public:
  /**
   * Performs any updating necessary for the background.
   * @param et The elapsed time, in milliseconds, since the last call to update().
   */
  virtual void update(float et) noth = 0;
  /**
   * Draws the background.
   */
  virtual void draw() noth = 0;
  /**
   * Draws any needed foreground component of the nebula.
   * Called after other drawing is completed.
   * Default does nothing.
   */
  virtual void postDraw() noth {}
  /**
   * Sets the internal reference, performing a full reset if requested.
   * @param obj The new reference; may be NULL
   * @param reset Whether to reset the view
   */
  virtual void updateReference(GameObject* obj, bool reset) noth = 0;
  /**
   * Reinitialises internal data.
   * For example, the StarField uses this to reset all Stars.
   * Default does nothing.
   */
  virtual void repopulate() noth {}
};

#endif /* BACKGROUND_HXX_ */
