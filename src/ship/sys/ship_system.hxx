/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ShipSystem class and related enums.
 */

#ifndef SHIP_SYSTEM_HXX_
#define SHIP_SYSTEM_HXX_

#include <GL/gl.h>

#include <vector>
#include <iostream>
#include <cstdlib>

#include "src/core/aobject.hxx"
#include "src/opto_flags.hxx"
#include "src/ship/ship.hxx"
#include "src/graphics/vec.hxx"

class Cell;
class Shield;

/** Simple clone() function for classes that only need a Ship to be constructed. */
#define DEFAULT_CLONE(class) virtual ShipSystem* clone() noth { return new class(parent); }
//Same as DEFAULT_CLONE, but includes orientation, if the class has a member with that name
/** Smiple clone() function for classes that only need a Ship to be constructed
 * and have an orientation member to copy.
 */
#define DEFAULT_CLONEWO(class) virtual ShipSystem* clone() noth { \
  class* c=new class(parent); \
  c->orientation = this->orientation; \
  return c; \
}

/** Classification of systems. */
enum Classification {
  Classification_None,  //!< No classification
  Classification_Power, //!< Power production
  Classification_Weapon,//!< Weapons
  Classification_Engine,//!< Thrust providers
  Classification_Shield,//!< ShieldGenerator
  Classification_Cloak, //!< CloakingDevice
};

/** Enumeration defining possible classes of weapons.
 */
enum Weapon { //Class C
              Weapon_EnergyCharge=0, Weapon_MagnetoBomb,
              //Class B
              Weapon_PlasmaBurst,  Weapon_SGBomb,
              //Class A
              Weapon_GatlingPlasma, Weapon_Monophase,
              Weapon_Missile, Weapon_ParticleBeam
};

/** Targetting modes for weapons. */
enum TargettingMode {
  ///"-1" for targetting mode, analogous to the integer protocol
  TargettingMode_NA,
  ///No targetting mode supported
  TargettingMode_None,
  ///Simply aim toward the ship.
  TargettingMode_Standard,
  ///Target the bridge
  TargettingMode_Bridge,
  ///Target the nearest power generator
  TargettingMode_Power,
  ///Target the nearest weapon system
  TargettingMode_Weapon,
  ///Target the nearest engine
  TargettingMode_Engine,
  ///Target nearest shield generator
  TargettingMode_Shield,
  ///Target nearest cell of any kind
  TargettingMode_Cell,
};

/** Data for the HUD to use when working with a weapon ShipSystem.
 * The system MUST at least fill in weapon speed. All bars are
 * defaulted to 0 by the HUD, while the colours are left undefined.
 *
 * Each bar defines a curve drawn to indicate an arbitrary value,
 * ranging from 0 to 1. Any solid colour may be specified.
 *
 * The dist value is not used in drawing, but is provided by the
 * HUD to indicate the expected travel distance before collision.
 */
struct WeaponHUDInfo {
  float speed;
  float topBarVal, botBarVal, leftBarVal, rightBarVal;
  vec3  topBarCol, botBarCol, leftBarCol, rightBarCol;
  float dist;
};

class ParticleBurst;

/** A ShipSystem (probably not just named System because of
 * my Java background) is anything like a power generator,
 * engine, et cetera.
 */
class ShipSystem : public AObject {
  friend class Ship;
  friend class TclShipSystem;
  protected:
  /** The Ship that contains the system. This is maintained
   * by external code; ShipSystem only sets it on construction.
   */
  Ship* parent;

  /** The beginning point at which any stealth transition
   * occurs.
   */
  int stealthTransitionPoint;

  /** Additional rotation, in radians, to be applied to the
   * system before drawing or penultimate translation.
   * Default is 0.
   */
  float autoRotate;

  public:
  /** Used internally by the networking stack.
   * Other code should not modify it.
   */
  unsigned netFutureOrientation;

  /** The texture used to draw this system. */
  const GLuint texture;

  /** The Cell that contains the system. This MUST be maintained
   * exclusively by external code; the constructor does not
   * even initialize it.
   */
  Cell* container;

  /** Defines the graphical positioning of the system.
   *
   * A centre system cannot coexist with any other system, and
   * is placed in the very centre of the cell. A forward system
   * can coexist only with a Standard or Backward Small system,
   * and is placed centred and frontwards. A backward system is
   * the same as forward, but is moved to the back.
   */
  enum Positioning { Standard, Centre, Forward, Backward } const positioning; //!< Positioning of system
  /** Defines the graphical size of the system.
   *
   * Two Large systems cannot share a cell, and a Large system must
   * be in a square or circle.
   */
  enum Size { Small, Large } const size; //!< Size of system

  /** System classification */
  const Classification clazz;
  /** Defines the type of a weapon.
   *
   * This is only meaningful if clazz is set to Classification_Weapon.
   */
  const Weapon weaponClass;

  protected:
  /**
   * System-specific update function.
   * May be NULL to indicate no updates.
   * Defaults to NULL.
   * @param that ShipSystem to operate on
   * @param et Elapsed time in milliseconds since previous call
   * @see note in src/ship/ship.hxx regarding updating of ShipSystems
   */
  void (*update)(ShipSystem*, float et) noth;

  /**
   * Constructs the ShipSystem with the given parms.
   * @param par Containing Ship
   * @param tex System texture to use
   * @param c System classification
   * @param p System positioning
   * @param s System size
   * @param wc Weapon class, if applicable (use anything if not a weapon)
   * @param up Update function
   */
  ShipSystem(Ship* par, GLuint tex,
              Classification c = Classification_None,
              Positioning p=Standard,
              Size s=Small,
              Weapon wc=Weapon_EnergyCharge,
              void (*up)(ShipSystem*, float) = 0)
  : parent(par), stealthTransitionPoint(400), autoRotate(0), texture(tex),
    positioning(p), size(s), clazz(c), weaponClass(wc), update(up),
    _supportsStealthMode(false) { }
  /*
  ShipSystem(Ship* par, GLuint tex, Classification c=Classification_None, Positioning p=Standard, Size s=Small,
             Weapon wc=Weapon_EnergyCharge)
  : parent(par), stealthTransitionPoint(400), autoRotate(0), texture(tex),
    positioning(p), size(s), clazz(c), weaponClass(wc), update(NULL),
    _supportsStealthMode(false) { } */

  /** Subclass must make this true if it works in stealth mode */
  bool _supportsStealthMode;

  public:
  /** Returns the power use (positive) or production (negative)
   * of this system when not in stealth.
   */
  virtual signed normalPowerUse() const noth = 0;
  /** Returns the power use (positive) or production (negative)
   * of this system when in stealth.
   *
   * By default, returns normalPowerUse() if _supportsStealthMode,
   * or 0 otherwise
   */
  virtual signed stealthPowerUse() const noth {
    return _supportsStealthMode? normalPowerUse() : 0;
  }

  /** Returns wheter the system supports running in stealth mode.
   * Physics will skip this if in stealth mode and this value
   * is false.
   */
  inline bool supportsStealthMode() const noth {
    return _supportsStealthMode;
  }

  /** Randomize the stealth transition point */
  void stealthTransition() noth {
    stealthTransitionPoint = rand()%STEALTH_COUNTER_RMAX;
  }

  /** Automatically determines the "best" orientation for the
   * system. If there is no possible orientation, return a
   * string describing the issue; otherwise return NULL.
   * It is assumed that, if this function returns NULL,
   * the positioning is valid for the Ship.
   * Default does nothing.
   */
  virtual const char* autoOrient() noth { return NULL; }

  /** Explicitly set the orientation of the system, with
   * a value as returned by getOrientation().
   * If the positioning is not valid (either the value itself
   * or the location in the Ship, oc), return an error message;
   * otherwise, NULL.
   * Default does nothing.
   */
  virtual const char* setOrientation(int) noth { return NULL; }

  /** Returns the orientation key for the system. The returned
   * value MUST be between 0 and 255 (inclusive), or be -1.
   * Return -1 if there is no meaningful value (default).
   */
  virtual int getOrientation() const noth { return -1; }

  /** Notifies the system that the parent Ship has just
   * changed (ie, to a different Ship* pointer). Default does nothing.
   */
  virtual void parentChanged() noth {}

  /** Instructs the system to perform location-specific
   * physics detection. This must always succeed; it is
   * called whenever a ship undergoes physical modification,
   * or creation. Default does nothing.
   */
  virtual void detectPhysics() noth {}

  /** This is to be called exactly once after the entire Ship
   * has been constructed. If it returns non-NULL, the Ship is
   * not valid due to this system. The intention of this is
   * for systems that depend on others within the Ship.
   *
   * This function will NOT be called in case of Ship damage
   * or fragmentation.
   */
  virtual const char* acceptShip() noth { return NULL; }

  /** Returns an exact copy of the given ShipSystem. */
  virtual ShipSystem* clone() noth=0;

  /** Returns the mass of the system. */
  virtual unsigned mass() const noth=0;

  /** Draws the system's visual representation onto
   * the Cell. GL has been translated and rotated
   * so that this should draw at 0,0.
   */
  void draw() const noth;

  /** Returns the amount to rotate the system
   * before final translation and drawing.
   */
  float getAutoRotate() const noth { return autoRotate; }

  /** Called when a ParticleBeam hits the cell containing the
   * system. Returns true if the system still exists.
   * Default returns true.
   */
  virtual bool particleBeamCollision(const ParticleBurst*) noth { return true; }

  /** Adds self to the given vector if the other weapon functions will respond
   * to this Weapon type.
   * Default does nothing.
   */
  virtual void weapon_enumerate(Weapon, std::vector<ShipSystem*>&) noth { }

  /** Returns the current energy level of the given weapon type.
   * Return -1 if the class is not applicable.
   */
  virtual int weapon_getEnergyLevel(Weapon clazz) const noth { return -1; }
  /** Returns the minimum allowed energy level of the weapon type.
   * Returns -1 if class not applicable.
   */
  virtual int weapon_getMinEnergyLevel(Weapon clazz) const noth { return -1; }
  /** Returns the maximum allowed energy level of the weapon type.
   * Returns -1 if class not applicable.
   */
  virtual int weapon_getMaxEnergyLevel(Weapon clazz) const  noth{ return -1; }
  /** Returns the name of the status icon for the weapon.
   * Return NULL if wrong class.
   */
  virtual const char* weapon_getStatus(Weapon clazz) const noth { return NULL; }
  /** Returns a human-readable short description (thirteen characters or fewer)
   * describing the weapon's status. Return NULL if wrong class.
   */
  virtual const char* weapon_getComment(Weapon clazz) const noth { return NULL; }
  /** Configures the WeaponHUDInfo appropriately and returns true
   * if applicable to the specified class; otherwise, return false.
   */
  virtual bool weapon_getWeaponInfo(Weapon clazz, WeaponHUDInfo&) const noth { return false; }
  /** Returns whether the weapon is ready to fire. Returns false
   * if the wrong class is used.
   */
  virtual bool weapon_isReady(Weapon clazz) const noth { return false; }
  /** Sets the energy level of the weapon. Does nothing if the wrong
   * class is passed.
   * Clamps level to defined bounds before setting.
   */
  virtual void weapon_setEnergyLevel(Weapon clazz, int level) noth {}
  /** Returns the energy required for one launch of the weapon.
   * Returns NaN if the class is incorrect.
   */
  virtual float weapon_getLaunchEnergy(Weapon clazz) const noth {
    //MSVC++ does not accept 0.0f/0.0f
    //float a=0.0f; return a/a;
    return NAN;
  }
  /** Increments the energy level of the weapon. */
  void weapon_incEnergyLevel(Weapon clazz) noth {
    int curr=weapon_getEnergyLevel(clazz);
    if (curr!=-1) weapon_setEnergyLevel(clazz, curr+1);
  }
  /** Decrements the energy level of the weapon. */
  void weapon_decEnergyLevel(Weapon clazz) noth {
    int curr=weapon_getEnergyLevel(clazz);
    if (curr!=-1) weapon_setEnergyLevel(clazz, curr-1);
  }
  /** Fires a weapon. */
  virtual void weapon_fire(Weapon clazz) noth {}

  /** Sets targetting mode for the given class. */
  virtual void weapon_setTargettingMode(Weapon clazz, TargettingMode mode) noth {}
  /** Returns the current targetting mode for the given class.
   * Return TargettingMode_NA if inappropriate.
   */
  virtual TargettingMode weapon_getTargettingMode(Weapon clazz) const noth { return TargettingMode_NA; }

  /** Adds a Shield* to the given vector
   * if the system is a shield generator.
   */
  virtual void shield_enumerate(std::vector<Shield*>&) noth {}

  /** If a ship can no longer support the power required by
   * shields, they are all shut down by a call to this
   * function.
   */
  virtual void shield_deactivate() noth {}

  /** Forces the shields to be shown, even if normal logic
   * would dictate otherwise. This is used by graphical
   * editors.
   */
  virtual void shield_elucidate() noth {}

  /** Returns a reinforcement factor that this system applies
   * to its container. This is not implemented as a ship-wide
   * function, since that would be meaningless.
   * The default is to return 1, indicating no change.
   */
  virtual float reinforcement_getAmt() noth { return 1; }

  /** Adds the containing Cell* to the vector if the system
   * is a damage disperser.
   */
  virtual void disperser_enumerate(std::vector<Cell*>&) noth {}

  /** Returns the amount of cooling the system provides, as a multiplier-minus-one.
   * Return 0.0 if none.
   */
  virtual float cooling_amount() noth { return 0.0f; }

  /** Returns true if the system integrates into the cooling
   * system (ie, it produces heat).
   */
  virtual bool heating_count() noth { return false; }

  /** Adds the container to any ship sound effects this system
   * produces.
   * Default does nothing.
   */
  virtual void audio_register() const noth { }

  /** Instructs a SelfDestructCharge to detonate. */
  virtual void selfDestruct() noth {}

  /** Called when the parent Cell is structurally damaged.
   * PowerPlants explode when this occurrs.
   * @param blame The cause of the damage
   */
  virtual bool damage(unsigned blame) noth {
    return true;
  }

  /** Called when the parent Cell is destroyed.
   * By default, just calls damage().
   * @param blame The cause of the destruction
   */
  virtual void destroy(unsigned blame) noth {
    damage(blame);
  }

  /** Serialize textually. This will be empty for most systems. */
  virtual void write(std::ostream&) noth {}
};

/** Activate the system texture shader. This is mainly used by cells
 * to draw their bridges.
 */
void activateShipSystemShader();

/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_enumerate(Ship*,Weapon,std::vector<ShipSystem*>&);
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
int weapon_getEnergyLevel(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
int weapon_getMinEnergyLevel(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
int weapon_getMaxEnergyLevel(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
const char* weapon_getStatus(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
const char* weapon_getComment(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_getWeaponInfo(Ship*, Weapon, WeaponHUDInfo&) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
bool weapon_isReady(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_setEnergyLevel(Ship*, Weapon, int) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
float weapon_getLaunchEnergy(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_incEnergyLevel(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_decEnergyLevel(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_fire(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void weapon_setTargettingMode(Ship*, Weapon, TargettingMode) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
TargettingMode weapon_getTargettingMode(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
bool weapon_exists(Ship*, Weapon) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void shield_enumerate(Ship*, std::vector<Shield*>&) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void shield_deactivate(Ship*) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void shield_elucidate(Ship*) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void disperser_enumerate(Ship*, std::vector<Cell*>&) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
float cooling_amount(Ship*) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void audio_register(Ship*) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
int heating_count(Ship*) noth;
/** Automatically call the system actions for all systems in the Ship,
 * according to protocols described for each action.
 * If no instances of the given class exist, return the specified
 * non-applicable value.
 */
void selfDestruct(Ship*) noth;

#endif /*SHIP_SYSTEM_HXX_*/
