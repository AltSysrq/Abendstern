/**
 * @file
 * @author Jason Lingle
 * @brief Contains the HumanController class and structures to use with it
 */

#ifndef HUMAN_CONTROLLER_HXX_
#define HUMAN_CONTROLLER_HXX_
#include <vector>
#include <string>

#include <SDL.h>
//Some older SDL libraries don't have X2, so just define it as
//the highest we can have
#ifndef SDL_BUTTON_X2
#define SDL_BUTTON_X2 SDL_BUTTON_WHEELDOWN
#endif

#include "controller.hxx"
#include "joystick.hxx"
#include "src/audio/ui_sounds.hxx"
#include "src/opto_flags.hxx"
#include "src/ship/sys/ship_system.hxx"

/**
 * Essentially an ActionDatum without the pair.
 * @see ActionDatum
 */
union SimpleActionDatum {
  float amt;
  void* ptr;
  long  cnt;
};
/** Essentially a std::pair<void*,SimpleActionDatum> that is a POD type.
 * Used to encapsulate data that have two values.
 */
struct ActionPair {
  void* first;
  SimpleActionDatum second;
};
/** Encapsulates data sent to actions. */
union ActionDatum {
  float amt;
  void* ptr;
  long  cnt;
  ActionPair pair;
};

/** A DigitalAction responds to discrete boolean events,
 * such as key and mouse-button presses/releases.
 */
struct DigitalAction {
  /** Passed to the function pointers. Contains internal data for them. */
  ActionDatum datum;
  /** If true, on is called repeadidly while the button is held */
  bool repeat;
  /** If true and repeat, on is called every frame. Otherwise, called every 200ms */
  bool fastRepeat;
  /** Called when the associated button is pressed. */
  void  (*on)(Ship*, ActionDatum& datum);
  /** Called when the associated button is released. */
  void (*off)(Ship*, ActionDatum& datum);
};

/** Responds to continuous events, such as mouse motion.
 * An AnalogueAction has a sensitivity, which is multiplied by the movement's
 * ratio, and a limit, which is the maximum of the absolute value of the
 * movement ratio after sensitivity.
 * It is also possible to have an AnalogueAction not recentre.
 */
struct AnalogueAction {
  /** The type of action this will perform. */
  enum Type { Rotation, EnginePower } type;
  /** Multiplied by motion amounts to determine the value passed to act. */
  float sensitivity;
  /** The maximum value to pass to act */
  float limit;
  /** If true, recentre the mouse on this axis. */
  bool recentre;

  /** Function to handle these events. */
  void (*act)(Ship*, float, bool recentres);
};

/** Length of compositionBuffer */
#define COMPOSITION_BUFFER_SZ 256
/** Buffer for the user to compose messages.
 */
extern char compositionBuffer[COMPOSITION_BUFFER_SZ];
/**
 * Tracks whether compositionBuffer is currently in use.
 * When isCompositionBufferInUse is true, all keyboard
 * input is expected to be redirected to composition, and the HUD should display
 * the message instead of the current target.
 * When false, the contents of the composition buffer are undefined.
 */
extern bool isCompositionBufferInUse;
/** The current index in compositionBuffer.
 * Only defined when isCompositionBufferInUse is true.
 */
extern unsigned compositionBufferIndex;

/**
 * Prefix to place before any chat messages posted by the user.
 */
extern std::string compositionBufferPrefix;

/** The HumanController takes key, button, and motion inputs and translates
 * them to actions. Rotation and engine power are considered "analogue" actions,
 * and everything else is "digital". An analogue action can be treated digital,
 * by having each event translate to a certain value.
 */
class HumanController: public Controller {
  //Don't spin more than once/frame
  bool spunThisFrame;

  //We ignore any ship within this vector when retargetting
  //and add the new target to it. If more than two seconds
  //have passed since the last retarget, we clear this
  //We never dereference any pointer in here, so it is
  //safe if any are deleted meanwhile
  //We also check once a second whether radar actually
  //detects the current target
  vector<Ship*> targetBlacklist;
  unsigned timeSinceRetarget;

  /* We need to accumulate multiple mouse-motions into one
   * so that limits work correctly.
   * Therefore, store the magnitudes here (using the
   * value derived from the LAST motion event, which
   * represents the total motion of the frame) and
   * only actually call the actions in update().
   */
  float horizMag, vertMag;

  //Used by motion to tell update where to warp the mouse
  int warpMouseX, warpMouseY;
  bool warpMouse;

  /* Toggles for warning sounds */
  audio::Toggle<audio::MapBoundaryWarning> mapBoundaryWarning;
  audio::Toggle<audio::CapacitorWarning> capacitorWarning;
  audio::Toggle<audio::PowerWarning> powerWarning;
  audio::Toggle<audio::HeatWarning> heatWarning;
  audio::Toggle<audio::HeatDanger> heatDanger;
  audio::Toggle<audio::ShieldUp> shieldUp;
  audio::Toggle<audio::ShieldDown> shieldDown;

  public:
  /** The currently selected Weapon. */
  Weapon currentWeapon;
  /** Contains all DigitalActions bound to the mouse buttons.
   *
   * Organized according to definitions in SDL.h.
   * The pressed bitmaps are global, since the
   * physical mouse and keyboard are logically global.
   */
  DigitalAction mouseButton[SDL_BUTTON_X2+1];
  /**Contains all DigitalActions bound to the keyboard.
   *
   * Organized according to definitions in SDL.h.
   * The pressed bitmaps are global, since the
   * physical mouse and keyboard are logically global.
   */
  DigitalAction keyboard[SDLK_LAST+1];

  AnalogueAction
    /** Action mapped to horizontal mouse movement. */
    mouseHoriz,
    /** Action mapped to vertical mouse movement. */
    mouseVert;

  struct JoystickBinding {
    std::vector<DigitalAction> buttons[JOYSTICK_NUM_BUTTON_TYPES];
    std::vector<AnalogueAction> axes[JOYSTICK_NUM_AXIS_TYPES];
  };
  std::vector<JoystickBinding> joysticks;

  /** Constructs a new HumanController to control the specified Ship.
   *
   * No actions are initially bound.
   */
  HumanController(Ship*);
  virtual ~HumanController();

  /** Add bindings from this controller to hc_conf */
  void hc_conf_bind();

  /** Called by the GameState running the game */
  void motion(SDL_MouseMotionEvent* e) noth;
  /** Called by the GameState running the game */
  void button(SDL_MouseButtonEvent* e) noth;
  /** Called by the GameState running the game */
  void key   (SDL_KeyboardEvent*    e) noth;

  virtual void update(float) noth;

  /** Returns the current target, or NULL if there is none. */
  Ship* getTarget() const noth {
    return (Ship*)ship->target.ref;
  }

  /** Request a change of target. */
  void retarget() noth;

  /** Semi-intelligently select the initial value for currentWeapon */
  Weapon selectFirstWeapon() const noth;

private:
  void handleJoystick(float) noth;
};

namespace action {
  //Standard digital actions
  extern const DigitalAction
    /** Accelerate. No repeat. No datum.*/
    accel,
    /**Decelerate. No repeat. No datum.*/
    decel,
    /**Turn. Repeat. Datum is float, the amount to turn per millisecond.*/
    turnLeft,
    /**Right is same as left, but with negative rotation*/
    turnRight,
    /**Increase thrust. No repeat. Datum is float, the amount to adjust. */
    accelMore,
    /**Decrease thrust. No repeat. Datum is float, the amount to adjust. */
    accelLess;

  extern const AnalogueAction
    /** Rotation action */
    rotate,
    /** Throttle action */
    throttle,
    /** Do-nothing action */
    noAction;
  /** Array of mouse buttons pressed. This should not be modified by external code. */
  extern bool buttonsPressed[SDL_BUTTON_X2+1];
  /** Array of keyboard keys pressed. This should not be modified by external code. */
  extern bool keysPressed[SDLK_LAST+1];
};

#endif /*HUMAN_CONTROLLER_HXX_*/
