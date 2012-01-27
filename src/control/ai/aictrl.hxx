/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AIControl class, the general-purpose computer ship controller
 */

/*
 * aictrl.hxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#ifndef AICTRL_HXX_
#define AICTRL_HXX_

#include <map>
#include <vector>
#include <string>
#include <deque>
#include <queue>

#include <libconfig.h++>

#include "src/control/controller.hxx"
#include "src/sim/objdl.hxx"

class AIModule;
class GameObject;
class Ship;

/** This is the primary AI controller.
 * @see src/control/ai/design.txt.
 */
class AIControl: public Controller {
  public:
  /** The number of weapons the AIControl understands. */
  static const unsigned numWeapons=7;

  /** The Variable type is used to encapsulate external module
   * variables.
   */
  struct Variable {
    /** The data type stored in the Variable. */
    enum Type { Invalid, Bool, Int, Float, String, Object } type;
    union {
      bool asBool;
      int asInt;
      float asFloat;
    };
    std::string asString;
    ObjDL asObject;
  };

  /** SingleWeaponInfo contains data on an individual launcher.
   * It stores
   * its angle of fire and the relative coordinates of the launcher,
   * relative to the ship at zero theta.
   */
  struct SingleWeaponInfo {
    float theta, relx, rely;
  };
  /** WeaponInfo contains all SingleWeaponInfos for a particular weapon
   * type.
   */
  typedef std::vector<SingleWeaponInfo> WeaponInfo;

  private:
  struct Module {
    AIModule* module;
    unsigned weight;
  };

  struct State {
    std::vector<Module> modules;
    unsigned totalWeight;
  };

  struct ModuleTimer {
    AIModule* module;
    float timeLeft;
    int arg;
  };

  struct Interrupt {
    libconfig::Setting* src;
    Variable oldValue;
    AIModule* listener;
    int arg;
  };

  std::map<std::string, State> states;
  libconfig::Setting* iaicInfo;
  std::string iaicInfoPrefix;

  State* currentState;

  typedef std::map<std::string, Variable> varmap;
  varmap perStateVariables;
  varmap globalVariables;
  WeaponInfo weaponInfos[numWeapons];

  std::queue<AIModule*> currentProcedure;

  std::deque<ModuleTimer> timers;
  std::vector<Interrupt> interrupts;

  float targetTheta;
  int currentWeapon;

  float timeSinceLastAction;

  void init(const libconfig::Setting&);

  public:
  /** Creates a new AIControl for the given Ship and setting information.
   * Throws an exception if the configuration is malformed.
   *
   * @param ship The ship to control
   * @param conf The root configuration for this AIControl
   * @param iaic The Inter-AI-Communication root, or NULL for none
   */
  AIControl(Ship* ship, const libconfig::Setting& conf, libconfig::Setting* iaic=NULL);
  /** Creates a new AIControl for the given Ship and setting information.
   * Throws an exception if the configuration is malformed.
   *
   * @param ship The ship to control
   * @param conf The root configuration for this AIControl
   * @param iaic The Inter-AI-Communication root, or NULL or empty for none
   */
  AIControl(Ship* ship, const char* conf, const char* iaic);

  virtual void update(float) noth;
  virtual ~AIControl();

  /** Returns the global variable of the given name.
   * Global variables persist through the lifetime of
   * the controller.
   * If it does not exist, it is created and initialized
   * to the Invalid type.
   */
  Variable& operator[](const char*);
  /** Returns the global variable of the given name.
   * If it does not exist, it is created and initialized
   * to the specified type and value. If it is the wrong
   * type, it is treated as if it did not exist (except
   * for int/float mismatch, which is transparently
   * converted).
   */
  bool gglob(const char*, bool);
  /** @see gglob(const char*,bool) */
  int gglob(const char*, int);
  /** @see gglob(const char*,bool) */
  float gglob(const char*, float);
  /** @see gglob(const char*,bool) */
  const char* gglob(const char*, const char*);
  /** @see gglob(const char*,bool) */
  GameObject* gglob(const char*, GameObject*);
  /** Sets the specified global variable.
   * If it does not exist, it is created automatically.
   * If it the wrong type, the type is changed before
   * assignment.
   */
  void sglob(const char*,bool);
  /** @see sglob(const char*,bool); */
  void sglob(const char*,int);
  /** @see sglob(const char*,bool); */
  void sglob(const char*,float);
  /** @see sglob(const char*,bool); */
  void sglob(const char*,const char*);
  /** @see sglob(const char*,bool); */
  void sglob(const char*,GameObject*);

  /** Returns the state variable of the given name.
   * State variables are discarded when states are changed.
   * If it does not exist, it is created and initialized
   * to the Invalid type.
   */
  Variable& operator()(const char*);
  /** Returns the state variable of the given name.
   * If it does not exist, it is created and initialized
   * to the specified type and value. If it is the wrong type,
   * it is treated as if it did not exist (except for int/
   * float mismatch, which is transparently converted).
   */
  bool gstat(const char*,bool);
  /** @see gstat(const char*,bool) */
  int  gstat(const char*,int);
  /** @see gstat(const char*,bool) */
  float gstat(const char*,float);
  /** @see gstat(const char*,bool) */
  const char* gstat(const char*,const char*);
  /** @see gstat(const char*,bool) */
  GameObject* gstat(const char*,GameObject*);
  /** Sets the specified state variable.
   * If it does not exist, it is created automatically.
   * If it is the wrong type, the type is changed before
   * assignment.
   */
  void sstat(const char*,bool);
  /** @see sstat(const char*,bool) */
  void sstat(const char*,int);
  /** @see sstat(const char*,bool) */
  void sstat(const char*,float);
  /** @see sstat(const char*,bool) */
  void sstat(const char*,const char*);
  /** @see sstat(const char*,bool) */
  void sstat(const char*,GameObject*);

  /** Returns a reference to the WeaponInfo for the specified
   * Weapon, cast to an int. Bounds checking is only performed
   * in debug mode.
   *
   * Access to WeaponInfo from Tcl should be handled by the intermediate
   * AIM_Tcl class, performing all bounds checking, et cetera.
   */
  WeaponInfo& getWeaponInfo(int);

  /** Returns a reference to the SingleWeaponInfo at the
   * specified indices. Bounds checking is only performed
   * in debug mode.
   */
  SingleWeaponInfo& getWeaponInfo(int,unsigned);

  /** Sets the state to the specified new state.
   * All state variables, timers, and interrupts
   * are discarded. If a proceedure is currently
   * executing, it is not aborted.
   * If the state does not exist, a warning message is
   * printed and no state change occurs.
   * This function does nothing if the specified state
   * is the current.
   */
  void setState(const char*);

  /** Adds the specified AIModule to the timer modules
   * list for the given number of milliseconds, which
   * is clamped to a limited amount.
   * The int argument is passed to AIModule::timer(int).
   */
  void addTimer(AIModule*, float, int);

  /** Cancels the specified AIModule/key timer pair.
   * If the pair does not exist, nothing happens.
   */
  void delTimer(AIModule*, int);

  /** Adds the specified AIModule to an interrupt based
   * on the specified IAIC subsetting.
   * Nothing happens if the setting does not exist or is
   * a non-scalar type.
   * The int argument is passed to AIModule::iaic(int).
   */
  void addInterrupt(AIModule*, const char*, int);

  /** Cancels the specified AIModule interrupt with
   * the specified int key.
   * Nothing happens if it does not exist.
   */
  void delInterrupt(AIModule*, int);

  /** Appends the specified module to the current procedure.
   */
  void appendProcedure(AIModule*);

  /** Returns the current target theta. */
  float getTargetTheta() const;
  /** Sets the target value for ship theta.
   * The update() function automatically tries its
   * best to guide the ship into this position.
   * Value is wrapped to legal values.
   */
  void setTargetTheta(float);

  /** Returns the current weapon. This is returned as
   * an int representation of the weapon, so that
   * ship_system did not need to be included into
   * this header.
   * This should not be exposed to Tcl; rather, the Tcl
   * intermediate module should provide a version that
   * returns an actual enumeration.
   */
  int getCurrentWeapon() const;

  /** Sets the current weapon.
   * If the integer is beyond supported range, the value
   * is ignored silently.
   * This should not be exposed to Tcl; rather, the Tcl
   * intermediate module should provide a version that
   * takes an actual enumeration.
   */
  void setCurrentWeapon(int);

  /** Returns true if IAIC information is available. */
  bool hasIAIC() const;

  /** Returns the IAIC information. The return value
   * is undefined if there is no such information.
   */
  libconfig::Setting& getIAIC();

  /** Returns the IAIC prefix, for Tcl to use with Confreg.
   * Returns empty string if no such information.
   */
  const char* getIAICPrefix() const;

  /** Type used to create AIModules by class name. */
  typedef AIModule* (*module_factory)(AIControl*, const libconfig::Setting&);

  /** Creates and returns a new AIModule from the given module type name.
   * If the type is not recognised, NULL is returned.
   * The behaviour of this function is undefined if registerAIModule
   * has not been called at least once.
   */
  AIModule* createAIModule(const char*, const libconfig::Setting&);

  /** Registers a new type of module.
   * THIS FUNCTION MUST BE CALLED AT LEAST ONCE BEFORE ANY
   * CALL TO createAIModule.
   */
  static void registerAIModule(const char*, module_factory);
};

/** The AIModuleRegistrar simplifies the process of registering
 * a new type of AIModule. The declaration <br>
 * &nbsp;  <code>static AIModuleRegistrar<AIM_MyModule> register("module_name");</code> <br>
 * is sufficient to allow the AIControl to know how to use the module.
 *
 * @param Mod The type to create in its factory function
 */
template<typename Mod>
class AIModuleRegistrar {
  public:
  /** Creates the registrar with the given name. */
  AIModuleRegistrar(const char* name) {
    AIControl::registerAIModule(name, &make);
  }

  private:
  static AIModule* make(AIControl* aic, const libconfig::Setting& s) {
    return new Mod(aic,s);
  }
};

#endif /* AICTRL_HXX_ */
