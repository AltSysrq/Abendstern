#ifndef HC_CONF_HXX_
#define HC_CONF_HXX_

/**
 * @file
 * @author Jason Lingle
 * @brief Contains functions to configure a human controller from abendstern.rc.
 *
 * All settings are stored under a root group, whose name is passed as
 * the second argument. Within, there is a group named "analogue"; it
 * contains a "horiz" and "vert" group, each of which have these options:<br>
 * &nbsp;  sensitivity: float
 * &nbsp;  autocentre: bool
 * &nbsp;  action: string ("none", "throttle", "rotate", for now)
 * It is up to HumanController to determine an appropriate limit (the default is copied).
 *
 * The "mouse" group contains the buttons: left, right, middle, up, down, x1, and x2.
 * The subgroups are optional. Within is "action" and "repeat", which simply bind
 * default digital actions and the repeat value. There is also an optional "value" entry,
 * which is placed into the datum field.
 *
 * The "keyboard" group contains subgroups named "key_###", where ### is the three-digit
 * keycode. The parms within are the same as for "mouse".
 */
#include <libconfig.h++>

#include "human_controller.hxx"

/** Structs and functions for operating HumanController configuration. */
namespace hc_conf {
  /** Defines the type of data from the "value" entry in the config */
  enum ActionParam {
    NoParam=0, Integer, Float, CString
  };
  /** Defines the limit for numeric values in the config */
  union ParamLimit {
    int intLimit;
    float floatLimit;
  };

  /** Default limit of 0 */
  static const ParamLimit defaultLimit={0};
  /** Returns a ParamLimit with floatLimit set to the requested value */
  inline ParamLimit getFloatLimit(float f) {
    ParamLimit l;
    l.floatLimit=f;
    return l;
  }
  /** Returns a ParamLimit with intLimit set to the requested value */
  inline ParamLimit getIntLimit(int i) {
    ParamLimit l;
    l.intLimit=i;
    return l;
  }

  /**
   * Configures the given HumanController according to the config. Logical errors are
   * printed to stderr but are otherwise ignored.
   * @param control The controller to configure
   * @param section The subsetting of conf["conf"] to use
   * @throw libconfig::ConfigException if there is a problem in the configuration itself.
   */
  void configure(HumanController* control, const char* section) throw (libconfig::ConfigException);

  /** Adds a new binding to a DigitalAction (which is copied).
   * Also specifies any parameter that can be passed. The paramater may
   * be a pair, in which case the value goes into the second, and the first
   * is copied from the original.
   */
  void bind(DigitalAction& act, const char* name, ActionParam parm=NoParam, bool paramPaired=false,
            ParamLimit limit=defaultLimit);

  /** Clears all action bindings.
   * @see bind()
   */
  void clear();
};

#endif /*HC_CONF_HXX_*/
