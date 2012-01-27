/**
 * @file
 * @author Jason Lingle
 * @brief Contains the HUD class and related interfaces
 */

#ifndef HUD_HXX_
#define HUD_HXX_

#include <GL/gl.h>

#include "camera.hxx"
#include "src/graphics/font.hxx"
#include "src/sim/objdl.hxx"
#include "src/ship/ship.hxx"
#include "src/opto_flags.hxx"

/* Up to 4 16-character messages may be presented to the user
 * via the integrated display. The below store the strings,
 * which MUST be NUL-terminated.
 */
/** The maximum length of one of the four HUD user messages.
 *
 * This does not include the terminating NUL char (so array
 * sizes must be one greater than this).
 *
 * This is larger than the actual space so that colour escapes may be used.
 */
#define MAX_HUD_USER_MESSAGE_LEN 63
namespace hud_user_messages {
  /** Message 0 to display on the HUD. */
  extern char msg0[MAX_HUD_USER_MESSAGE_LEN+1],
  /** Message 1 to display on the HUD. */
              msg1[MAX_HUD_USER_MESSAGE_LEN+1],
  /** Message 2 to display on the HUD. */
              msg2[MAX_HUD_USER_MESSAGE_LEN+1],
  /** Message 3 to display on the HUD. */
              msg3[MAX_HUD_USER_MESSAGE_LEN+1];
  /** Sets the specified HUD user message.
   *
   * Errors are silently
   * ignored (such as the string being longer than
   * MAX_HUD_USER_MESSAGE_LEN or the index being out of range).
   *
   * @param ix The message to set; must be 0, 1, 2, or 3
   * @param str The string to set it to; must be 0 to MAX_HUD_USER_MESSAGE_LEN
   * characters long, inclusive, not including the terminating NUL
   */
  void setmsg(unsigned ix, const char* str);
}

/** The heads-up display informs the player about the various stati
 * of their ship and their target. It is in the camera/ directory
 * since it is really closer to being part of the camera than
 * anything else.
 *
 * The HUD assumes it has the stensil buffer to itself. If any other
 * code modifies it while the HUD is in use, odd things may happen.
 */
class HUD {
  private:
  ObjDL ship;

  Font& font, &smallFont;

  /* We make the HUD flicker upon significant hull damage. The probability
   * of the HUD functioning in a given frame is equal to
   *   currentStrength / originalStrength)
   * but will never be reduced below 0.1.
   *
   * Additionally, the HUD lines are reddened to indicate recent damage,
   * based on how recently the damage occurred.
   */
  float currentStrength, originalStrength;
  float timeSinceDamage;

  /* When this reaches zero, we clear and resetup the stensil buffer
   * for the minimap, then reset it to 600. It is decremented every
   * frame. This periodic refresh is to account for the buffers being
   * destroyed if the application ever loses focus.
   */
  unsigned stensilBufferSetup;

  public:
  /** If true, a full-screen map will be displayed instead of the normal HUD. */
  bool fullScreenMap;

  /**
   * Constructs a new HUD for the given ship and field.
   * @param s The initial ship to work with, which may be NULL
   * @param f The field to operate within
   */
  HUD(Ship* s,GameField* f);
  /**
   * Performs internal updating.
   * @param et The elapsed time, in milliseconds, since the previous call to update()
   */
  void update(float et) noth;
  /**
   * Draws the HUD.
   * @param cam The current Camera. Currently, the HUD is only fully functional if
   * this is a DynamicCamera.
   */
  void draw(Camera* cam) noth;
  /**
   * Alters the ship reference.
   * @param s The new Ship to track, which may be NULL.
   */
  void setRef(Ship* s) noth;
  /**
   * Informs the HUD that the ship has been damaged.
   * @param amt Damage applied (currently ignored)
   */
  void damage(float amt) noth;

  /**
   * Adds all bindings to hc_conf.
   */
  void hc_conf_bind();

  private:
  void drawChat() noth;
};

#endif /*HUD_HXX_*/
