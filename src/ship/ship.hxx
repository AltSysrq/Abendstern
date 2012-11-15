/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Ship class
 */

#ifndef SHIP_HXX_
#define SHIP_HXX_

#include <list>
#include <vector>
#include <cmath>
#include <utility>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <cassert>
#include <GL/gl.h>
#include <SDL.h>

#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/opto_flags.hxx"
#include "src/sim/objdl.hxx"
#include "src/sim/lazy_transform_collision_tree.hxx"
#include "src/secondary/aggregate_set.hxx"
#include "src/audio/ship_effects_common.hxx"
#include "physics_bits.hxx"

/** The maximum amount to multiply elapsed time by in an argument
 * to Ship::spin(). */
#define STD_ROT_RATE (1/250.0f)
/** The lowest temperature a ship can have */
#define ROOM_TEMPERATURE 293.15f
/** The radius multiplier to determine from how far a non-stealthed
 * ship can be detected on radar.
 */
#define UNSTEALTH_VISIBILITY 500.0f
/** The radius multiplier to determine from how far a stealthed,
 * uncloaked ship can be detected on radar.
 */
#define STEALTH_VISIBILITY 50.0f
/** The radius multiplier to determine from how far a cloaked ship
 * can be detected on radar.
 */
#define CLOAK_VISIBILITY 5.0f
/** Maximum temperature a ship can reach.
 * This is 100 Kelvins under the sublimation point of graphite.
 */
#define MAX_TEMP (3725+273.15f) //100 K less than sublimation point of graphite
/** The temperature at which to begin warning the user about heat. */
#define WARN_TEMP (MAX_TEMP-256)
/** The lower bound of the stealth counter */
#define STEALTH_COUNTER_MIN 0
/** The upper bound of stealth counter random trigger points */
#define STEALTH_COUNTER_RMAX 400
/** The upper bound of the stealth counter */
#define STEALTH_COUNTER_MAX 500

/** The maximum number of Cells a Ship is allowed to have. */
#define MAX_CELL_CNT 4094

class Blast;
class ShipSystem;
class Shield;
class Cell;
class Engine;
class PlasmaBurstLauncher;
class GatlingPlasmaBurstLauncher;
class ShipRenderer;
class Controller;
class EffectsHandler;
class ShipDamageGeraet;

typedef AggregateSet<Ship*,Ship*> radar_t;

/**
 * Represents a spacecraft.
 *
 * It is by far the most important and most complex class in the game, due
 * to the flexibility it must have to deal with user-supplied ship designs
 * and the requirement to do so efficiently.
 */
class Ship: public GameObject {
  friend void audio::shipSoundEffects(float,Ship*);
  friend class INO_Ship4;
  friend class ENO_Ship4;
  friend class INO_Ship8;
  friend class ENO_Ship8;
  friend class INO_Ship16;
  friend class ENO_Ship16;
  friend class INO_Ship64;
  friend class ENO_Ship64;
  friend class INO_Ship256;
  friend class ENO_Ship256;
  friend class INO_Ship1024;
  friend class ENO_Ship1024;
  friend class INO_Ship4094;
  friend class ENO_Ship4094;
  public:
  /** The possible categories returned by Ship::categorise().
   *
   * Each category is defined to be effective against two other categories; it
   * by this definition that ship quality is judged.
   */
  enum Category {
    /**
     * Swarm ships are small and agile, but weak. They are effective when
     * working together.
     *
     * Effective against: Fighter, Attacker
     */
    Swarm=0,
    /**
     * Interceptors are fighters with an emphasis on acceleration, intended to
     * eliminate escort fighters and swarms.
     *
     * Effective against: Swarm, Fighter
     */
    Interceptor,
    /**
     * General-purpose fighters, usable as escorts and for removing heavier
     * craft.
     *
     * Effective against: Defender, Attacker
     */
    Fighter,
    /**
     * Heavier than fighters, and armed enough to be effective against larger
     * ships, but still manoeuverable enough to defend themselves.
     *
     * Effective against: Defender, Subcapital
     */
    Attacker,
    /**
     * Very large, but non-capital, ships which have large amounts of
     * fire-power, but can still get around well enough. Their strength must
     * render lighter craft ineffective.
     *
     * Effective against: Swarm, Interceptor
     */
    Subcapital,
    /**
     * Larger craft with a focus on fire-power and rotation instead of movement.
     *
     * Effective against: Attacker, Subcapital
     */
    Defender
  };

  /** Cells arranged according to network indices.
   * If this is empty, this operation has not yet been done.
   *
   * EmptyCells are not included, and there may be NULLs.
   */
  std::vector<Cell*> networkCells;

  /** All Cells present in the Ship */
  std::vector<Cell*> cells;
  /** The current Controller, which may be NULL. */
  Controller* controller;
  /** The current EffectsHandler for this Ship */
  EffectsHandler* effects;
  /** The current target of this Ship's controller */
  ObjDL target;
  /** The name of the mountpoint from which the ship was loaded. */
  std::string typeName;

  /**
   * Due to a performance improvement.
   *
   * Many ships have lots of engines. Calling the update function for
   * each of them results in much redundancy and wasted time.
   * Therefore, store common information here, and call only
   * one function to loop through them.
   */
  struct EngineInfo {
    friend class ::Engine;
    friend class Ship;
    private:
    EngineInfo() : physicalOnTime(0), timeSinceLastExpsosion(0), fade(0) {}
    EngineInfo(const EngineInfo& e)
    : physicalOnTime(e.physicalOnTime),
      timeSinceLastExpsosion(e.timeSinceLastExpsosion),
      fade(e.fade)
    { }
    //Not defined --- DO NOT USE!
    void operator=(const EngineInfo&);

    std::vector<Engine*> list;
    /* Since we only create trails on the last
     * virtual frame, we use this to sum up
     * the amount of time the engine was
     * on during the physical frame.
     */
    float physicalOnTime;

    //Don't create explosions less than 50 ms apart
    float timeSinceLastExpsosion;

    /* We fade the animation in/out when the
     * engine is turned on/off.
     */
    float fade;

    public:
    /** Fading between on/off states (0=off,1=on) */
    inline float getFade() { return fade; }
  } engineInfo;

  /**
   * Due to performance improvement.
   *
   * Same rationale as for the EngineInfo struct.
   */
  struct PlasmaBurstLauncherInfo {
    friend class ::PlasmaBurstLauncher;
    friend class ::GatlingPlasmaBurstLauncher;
    friend class Ship;

    PlasmaBurstLauncherInfo()
    : count(0)
    {}
    PlasmaBurstLauncherInfo(const PlasmaBurstLauncherInfo& p)
    : count(p.count)
    { }

    private:
    int count;
  } plasmaBurstLauncherInfo;

  /** Keeps track of the current temperature and cooling rate. */
  struct HeatInfo {
    friend class Ship;
    friend class ::PlasmaBurstLauncher;
    friend class ::GatlingPlasmaBurstLauncher;

    HeatInfo()
    : temperature(ROOM_TEMPERATURE), coolRate(1)
    {}
    HeatInfo(const HeatInfo& h)
    : temperature(h.temperature), coolRate(h.coolRate)
    { }

    private:
    float temperature, coolRate;
  } heatInfo;

  //Projects a coordinate relative to a cell according
  //to the ship and the cell's offset
  static std::pair<float,float> cellCoord(const Ship* parent, const Cell* cell) noth HOT;
  static std::pair<float,float> cellCoord(const Ship* parent, const Cell* cell, float x, float y) noth HOT;

  private:
  ShipRenderer* renderer;

  /* Iterating through the cells like this:
   *   for (int i..) {
   *     if (cells[i]->systems[0]) cells[i]->systems[0]->update(et);
   *     if (cells[i]->systems[1]) cells[i]->systems[1]->update(et);
   *   }
   * Takes too long, especially given that almost all ShipSystems
   * simply do nothing in the update. (Namely, it takes 20% of
   * execution time.) Therefore, do it a faster way that avoids
   * virtual calls and unnecessary function calls.
   */
  struct SystemUpdate {
    ShipSystem* sys;
    void (*call)(ShipSystem*, float) noth;

    void operator()(float et) const noth { call(sys, et); }
  };
  std::vector<SystemUpdate> cellUpdateFunctions;

  /* The currently-valid physical information.
   * The CELL bits are 1 if all cells are known to have valid information
   * here, or 0 if any might not.
   */
  physics_bits validPhysics;
  /* Used for debugging.
   * When set, some part of the Ship code is assuming that physics properties
   * will not be invalidated.
   */
  bool physicsLocked;
  friend class SetPhysicsLockedInScope;

  /* Total mass of the ship. */
  float mass;
  /* Determined by multiplying the distance from the centre
   * of gravity by the mass of respective cells.
   */
  float angularInertia;

  /* The colour of the ship */
  float colourR, colourG, colourB;

  /* Rotation and rotational velocity */
  float theta, vtheta;
  /* Cached results of cos(theta) and sin(theta) */
  float cosTheta, sinTheta;

  /* If true, this is actually a fragment of a ship,
   * so it will just spin and drift.
   */
  bool isFragment;

  /* Same power values as in Cell::PhysicalInformation, but
   * the sum for the entire ship.
   */
  signed powerUC, powerUT, powerSC, powerST;
  unsigned ppowerU, ppowerS;

  /* The current power values */
  unsigned currPowerDrain, currPowerProd;

  /* The maximum amount of capacitance we can have. */
  unsigned totalCapacitance;
  /* The current capacitance. */
  float currentCapacitance;

  /* The percentage of maximum thrust used by the engines,
   * as well as whether thrust and/or brake are on.
   */
  float thrustPercent;
  bool thrustOn, brakeOn;

  /* The magnitude of the thrust at 100% throttle. */
  float thrustMag;
  /* The vector of the thrust at 100% throttle. */
  float thrustX, thrustY;
  /* The torque exerted by thrust at 100% throttle. */
  float thrustTorque;
  /* The rotational force the engines can apply */
  float rotationalThrust;

  /* Computed by taking the farthest distance
   * from the bridge to any cell.
   */
  float radius;

  /* This is similar to gradius, but measures
   * the half-edge needed to put a square around
   * the Ship. That is, it is the maximum individual
   * coordinate of any cell.
   */
  float boundingSquareHalfLength;

  /* The percent reinforcement the ship has.
   * The strength of each cell is base*(1+reinforcement);
   * The mass of each is base*(1+reinforcement*reinforcement).
   */
  float reinforcement;

  /* If true, calls to drawPower() always return false,
   * but the input values are summed up and retrievable
   * from endTest().
   */
  bool powerTest;
  float powerTestSum;

  /* This is increased every frame that a fragment
   * is not visible. If it becomes visible, this
   * is set to 0.
   * The purpose of this is to prevent the progressive
   * accumulation of slow-moving ship parts.
   */
  float invisibleFragTime;

  /* The amount of time since the last call to draw().
   * When this exceeds 5000, we free all damage textures
   * associated with the Ship's Cells.
   */
  float invisibleTime;

  /* If we ever need to deactivate shields to conserve
   * power, we set this, so we don't try again.
   */
  bool shieldsDeactivated;

  /* See note in constructor of Shield in ship/shield.hxx.
   * This is where we store all the shields associated
   * with us.
   */
  std::vector<Shield*> shields;

  /* This is just a cache of the value returned by
   * cooling_amount divided by the value returned
   * by heating_count.
   */
  float coolingMult;
  /* Store these so we can perform deltas properly. */
  float rawCoolingMult;
  unsigned heatingCount;

  /* Whether the Ship is currently in Stealth mode.
   * When true, all non-stealth systems are shut down.
   * The ship will only exit stealth mode automatically
   * if all other options (other than permanently
   * deactivating shields, which are already inactive)
   * have been exhausted.
   */
  bool stealthMode;

  /* Used for a few stealth entry and exit effects. */
  int stealthCounter;

  /* Set to true if we have a running cloaking device. */
  bool hasCloakingDevice;

  /* Counts the time until a slow-fire weapon may be used
   * again.
   */
  float timeUntilSlowFire;

  std::vector<NebulaResistanceElement> nebulaResistanceElements;

  /* All ships connected to the same radar share sensor
   * information. Every five to ten seconds, a Ship refreshes
   * its sensors; detection rules are as follows:
   *   Non-stealth: Visible up to 500*radius screens
   *   Stealth:     Visible up to 50*radius screens
   *   Cloaked:     Visible up to 5*radius screens
   *   Dead:        Not included on radar
   */
  radar_t defaultRadar;
  radar_t* radar;
  float timeUntilRadarRefresh;

  /* Quickly map Weapon types to the ShipSystems that respond to them. */
  std::vector<ShipSystem*> weaponsMap[8];

  /* The Cells containing DispersionShields. */
  std::vector<Cell*> cellsWithDispersionShields;

  /* Collision tree */
  LazyTransformCollisionTree collisionTree;

  /* Set to true if this ship interacts with the ShipMixer.
   * Default is false.
   * Only one ship at a time may have this true at a time.
   */
  bool soundEffects;

  /* Maintains a "blame counter" for various blameids that have damaged the ship.
   * Every time the ship is damaged, that damage is added to the appropriate
   * slot's sum (if no slot for that blame exists yet, it replaces the one with
   * the least sum).
   *
   * Every ten seconds, every sum's value halves (this is progressive).
   */
  struct damage_blame_t {
    unsigned blame;
    float damage;
  } damageBlame[4];
  friend bool damageBlameCompare(const damage_blame_t&, const damage_blame_t&);

  /* A list of Blast*s that the current invocation of collideWith() is applying
   * to. It is meaningless if collideWith() is not currently on the stack.
   *
   * This is used to inject new Blasts into the current invocation, to avoid
   * reëntrancy and the performance problems with the simple "let the GameField
   * handle it" solution.
   */
  std::vector<Blast*> collideWithCurrent;
  /* A list of Blast*s which have been injected into collideWith(). Calls to
   * collideWith() which refer to any of these are ignored.
   */
  std::list<ObjDL> injectedBlasts;

  public:
  /** The insignia (or factional alliance) of the ship. Ships with
   * the same insignia are together; 0 is neutral;
   * other non-matching values may be apposed or allied.
   * By default, this is reinterpret_cast<unsigned long>(this).
   */
  unsigned long insignia;

  /** The blame that identifies the ship's owner, for scoring purposes.
   *
   * Exactly what this means is particular to the shell, but must be
   * less than 256 for local ships for the value to be preserved over the network.
   *
   * Defaults to 0xFFFFFF.
   */
  unsigned blame;

  /**
   * The amount the ship has scored during its lifetime.
   * This must be maintained externally.
   *
   * Defaults to 0.
   */
  signed score;

  /**
   * The score of the player piloting the ship. Maintained externally.
   *
   * Defaults to 0.
   */
  signed playerScore;

  /**
   * If true, the Ship was killed with spontaneouslyDie().
   */
  bool diedSpontaneously;

  /**
   * Amount to multiply all damage the ship /takes/ by.
   * This is primarily intended to be used to set difficulty
   * in single-player modes.
   * Defaults to 1.
   */
  float damageMultiplier;

  /** This function is called when the ship is being
   * destroyed (or for some other reason ceases to
   * exist). Beware that the Ship may be deleted
   * shortly after this function returns. This
   * is indicated by the second value.
   */
  void (*shipExistenceFailure)(Ship*, bool death);

  /**
   * If a remote ship, the ShipDamageGeraet to send damage information through.
   */
  ShipDamageGeraet* shipDamageGeraet;

  /** Creates a new, empty Ship for the given GameField. */
  Ship(GameField*);
  /** Makes a deep copy of the other ship. */
  Ship(const Ship& other);

  /** Sets x, y, and theta to 0, and returns data to restore
   * the previous state.
   * The contents of this data is undefined.
   */
  float* temporaryZero() noth;
  /** Restores state from temporaryZero(), and deletes the
   * incomming pointer.
   */
  void restoreFromZero(float*) noth;

  /** Clears the bits indicating that the specified physics
   * information is valid.
   * Even if (bits & PHYS_CELL_MASK) is non-zero, no cells are
   * informed about this.
   */
  void physicsClear(physics_bits pb) noth {
    assert(!physicsLocked);
    validPhysics &= ~pb;
  }
  /** Ensures that the specified physical calculations are
   * up-to-date.
   */
  void physicsRequire(physics_bits pb) noth;

  /** Returns the Cell* nearest the one passed in. The float& is
   * set to the distance between the two.
   * If there are no dispersion shields, NULL is returned, and
   * the float& is unchanged.
   * This ASSUMES that the PHYS_SHIP_DS_INVENTORY_BIT is
   * already valid. (An assert checks it in the DEBUG build).
   */
  Cell* nearestDispersionShield(const Cell*, float&) const noth;

  /** Rescans the ShipSystems for changes in update functions. */
  void refreshUpdates() noth;

  #ifndef AB_OPENGL_14
  /**
   * Prepares the given ShipRenderer's dynamic colour pallet.
   *
   * The ShipRenderer need not belong to this Ship.
   */
  void preparePallet(ShipRenderer*) const;
  #endif

  virtual bool update(float) noth HOT;
  virtual void draw() noth;
  virtual float getRotation() const noth { return theta; }
  /** Returns the cosine of the current rotation. */
  inline float getCos() const noth { return cosTheta; }
  /** Returns the sine of the current rotation. */
  inline float getSin() const noth { return sinTheta; }
  /** Returns the rotational velocity of the Ship */
  float getVRotation() const noth { return vtheta; }
  virtual float getRadius() const noth;
  /** Returns the mass of the Ship */
  float getMass() const noth { const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_MASS_BIT); return mass; }

  virtual void teleport(float x, float y, float theta) noth {
    this->x=x;
    this->y=y;
    this->theta=theta;
    cosTheta = cos(theta);
    sinTheta = sin(theta);
  }

  /**
   * Sets the OpenGL colour.
   * This is only meaningful in the OpenGL2.1-compatibility mode now,
   * as it calls glColor4f.
   * @param intensity Multiplier for R, G, and B components
   * @param alpha Alpha to set the colour to
   */
  inline void glSetColour(float intensity=1, float alpha=1)  noth{
    glColor4f(colourR*intensity, colourG*intensity, colourB*intensity, alpha);
  }

  /**
   * Alters the current thrust.
   * @param t The amount to set it to. If <= 0, the call is ignored. It is capped to 1.
   */
  void setThrust(float t) noth {
    if (t>0) thrustPercent=(t<=1? t : 1);
    updateCurrPower();
  }
  /** Returns the current effective thrust.
   * Specifically, returns zero when !isThrustOn() and getTrueThrust() otherwise.
   */
  float getThrust() const noth {
    return (thrustOn? thrustPercent : 0);
  }
  /** Returns the current thrust amount set by setThrust(), regardless of isThrustOn(). */
  float getTrueThrust() const noth {
    return thrustPercent;
  }
  /** Alters whether forward thrust is enabled. */
  void setThrustOn(bool o) noth {
    thrustOn=o;
    if (!thrustOn) engineInfo.fade=0;
    if (validPhysics & PHYS_SHIP_POWER_BIT)
      updateCurrPower();
  }
  /** Returns whether forward thrust is enabled. */
  bool isThrustOn() const noth {
    return thrustOn;
  }
  /** Alters whether the brake is enabled. */
  void setBrakeOn(bool o) noth {
    brakeOn=o;
    if (validPhysics & PHYS_SHIP_POWER_BIT)
      updateCurrPower();
  }
  /** Returns whether the brake is enabled. */
  bool isBrakeOn() const noth {
    return brakeOn;
  }
  /**
   * Simultaneously alters the state of forward thrust,
   * brake, and thrust amount.
   *
   * For use by AI, which often set all three together.
   */
  void configureEngines(bool thrust, bool brake, float amt) noth {
    if (thrustOn==thrust && brakeOn==brake && amt==thrustPercent) return;
    thrustOn=thrust;
    if (!thrustOn) engineInfo.fade=0;
    brakeOn=brake;
    thrustPercent=(amt >= 0? amt <= 1? amt : 1 : 0);
    if (validPhysics & PHYS_SHIP_POWER_BIT)
      updateCurrPower();
  }
  /** Like configureEngines(bool,bool,float), but does not alter
   * the current thrust amount.
   */
  inline void configureEngines(bool t, bool b) noth {
    configureEngines(t, b, thrustPercent);
  }

  /** Returns the angle in which net thrust occurs */
  inline float getThrustAngle() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_THRUST_BIT);
    return std::atan2(thrustY,thrustX);
  }

  /** Returns the current stealth mode */
  inline bool isStealth() const noth {
    return stealthMode;
  }
  /** Returns the current stealth counter */
  inline int getStealthCounter() const noth {
    return stealthCounter;
  }

  /** Enters or exits stealth mode */
  void setStealthMode(bool) noth;

  /** Toggles stealth mode */
  inline void toggleStealthMode() noth {
    setStealthMode(!stealthMode);
  }

  /** Returns whether the ship has an active cloaking device */
  inline bool isCloaked() const noth { return hasCloakingDevice; }

  /** Returns true if the ship is ready for a slow-fire weapon. */
  inline bool slowFireReady() const noth { return timeUntilSlowFire <= 0; }
  /** Resets the slow fire counter to the specified value. */
  inline void setSlowFire(float t) noth { timeUntilSlowFire = t; }

  /** Returns the currently-used radar, or NULL
   * if the default is in use.
   */
  radar_t* getRadar() const noth;
  /** Sets the current radar. NULL indicates
   * to use the default, internal radar.
   */
  void setRadar(radar_t*) noth;
  /** Sets the begin and end radar iterators. */
  void radarBounds(radar_t::iterator& begin, radar_t::iterator& end) noth;

  /** Returns the acceleration of the Ship. Returned
   * value is in screen/ms/ms. */
  float getAcceleration() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_THRUST_BIT);
    return thrustMag/mass;
  }

  /** Calculates the maximum rotation rate (if calling spin
   * with STD_ROT_RATE for 10 seconds). Unlike getAcceleration(),
   * this is currently an expensive operation.
   *
   * Returns rad/ms.
   */
  float getRotationRate() const noth;

  /** Returns the base rotational acceleration (not including
   * slowdown). Returns values in rad/ms/ms.
   */
  float getRotationAccel() const noth;

  /**
   * Returns the rotational acceleration incurred by engine imbalance
   * (ie, uncontrolled rotation).
   */
  float getUncontrolledRotationAccel() const noth;

  /** Returns half the length of an edge of the Ship's bounding square. */
  float getBoundingSquareHalfEdge() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_COORDS_BIT);
    return boundingSquareHalfLength;
  }

  /** Returns the current percent (0..1) of the Ship's power usage.
   * May be > 1 if using more power than is produced.
   */
  float getPowerUsagePercent() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_POWER_BIT);
    if (currPowerProd)
      return currPowerDrain/(float)currPowerProd;
    else
      return 0.0f;
  }

  /** Returns how much power is currently being produced. */
  unsigned getPowerSupply() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_POWER_BIT);
    return currPowerProd;
  }
  /** Returns how much power is currently being drained. */
  unsigned getPowerDrain() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_POWER_BIT);
    return currPowerDrain;
  }

  /** Returns the current capacitance amount. */
  float getCurrentCapacitance() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_CAPAC_BIT);
    return currentCapacitance/1000.0f;
  }
  /** Returns the maximum possible capacitance amount. */
  unsigned getMaximumCapacitance() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_CAPAC_BIT);
    return totalCapacitance;
  }
  /** Returns the current percent (0..1) of capacitance. */
  float getCapacitancePercent() const  noth{
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_CAPAC_BIT);
    if (totalCapacitance)
      return currentCapacitance/totalCapacitance;
    else
      return 0;
  }

  /** Sets the colour of the Ship. */
  void setColour(float r, float g, float b) noth {
    colourR=r; colourG=g; colourB=b;
  }

  /** Returns the red component of the Ship's colour. */
  float getColourR() const noth { return colourR; }
  /** Returns the green component of the Ship's colour. */
  float getColourG() const noth { return colourG; }
  /** Returns the blue component of the Ship's colour. */
  float getColourB() const noth { return colourB; }

  /** Withdraws the given amount of power from the capacitors.
    * @return true if it was successful, false otherwise
    */
  bool drawPower(float amount) noth;

  /** Returns the reinforcement amount of the Ship. */
  float getReinforcement() const noth {
    return reinforcement;
  }
  /** Sets the reinforcement amount of the Ship.
   * This needs to be done before the Ship has performed ANY physics calculations.
   */
  void setReinforcement(float r) noth {
    reinforcement=r;
  }

  /** Returns whether the Ship is alive */
  inline bool hasPower() const noth {
    return !isFragment;
  }

  /** Returns the base cooling multiplier */
  float getCoolingMult() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_COOLING_BIT);
    return coolingMult;
  }

  /** Returns the current heat percent (temperature/MAX_TEMP). */
  float getHeatPercent() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_COOLING_BIT);
    return heatInfo.temperature / MAX_TEMP;
  }

  /** Causes the ship to spontaneously lose power.
   * This only has effect if the ship is currently
   * powered and is not remote.
   */
  void spontaneouslyDie() noth;

  /** Returns the internal vector of shields. */
  const std::vector<Shield*>& getShields() const noth {
    const_cast<Ship*>(this)->physicsRequire(PHYS_SHIP_SHIELD_INVENTORY_BIT);
    return shields;
  }

  /** Returns the velocity of the given Cell's centre. */
  std::pair<float,float> getCellVelocity(const Cell*) const noth;

  /** Returns a vector of ShipSystems that will respond to the given
   * Weapon type.
   */
  const vector<ShipSystem*>& getWeapons(unsigned) const noth;

  /** If there is currently a ShipRenderer, notify it that the
   * given cell has been damaged.
   */
  void cellDamaged(Cell* c) noth;
  /** If there is currently a ShipRenderer, notify it that
   * a cell is physically modified.
   */
  void cellChanged(Cell*) noth;

  /**
   * Informs the Ship about drastic graphics alteration.
   *
   * Informs the ship that its graphics information has changed
   * drastically enough that it must completely refresh it, including
   * cell assignment.
   */
  void destroyGraphicsInfo() noth;

  /** Enables this ship to use the ShipMixer. */
  void enableSoundEffects() noth;
  /** Disables this ship's interaction with the ShipMixer.
   * This also calls the ShipMixer::reset function.
   */
  void disableSoundEffects() noth;
  /** Returns whether this Ship has sound effects enabled. */
  bool soundEffectsEnabled() const noth { return soundEffects; }

  //Control
  private:
  void accel(float et) noth
  #ifdef PROFILE
  __attribute__((noinline)) //We don't want these to interfere with the profile
  #endif
  ;
  void decel(float et) noth
  #ifdef PROFILE
  __attribute__((noinline))
  #endif
  ;
  public:
  /** Rotates the Ship with the given amount of rotational power. */
  void spin(float amount) noth
  #ifdef PROFILE
  __attribute__((noinline))
  #endif
  ;

  /** Sets test mode.
   * This means that that all calls to drawPower() will return false.
   * Input values will be summed up and returned by endTest(). This allows
   * AI to determine whether the action can succeed.
   */
  void startTest() noth;
  /** Ends test mode.
   * @return the amount of power required for anything that
   * happened during.
   */
  float endTest() noth;

  virtual const vector<CollisionRectangle*>* getCollisionBounds() noth;
  virtual bool isCollideable() const noth { return true; }
  virtual CollisionResult checkCollision(GameObject*) noth;
  virtual bool collideWith(GameObject*) noth;

  /**
   * "Injects" the given Blast into the currently-running collideWith()
   * invocation. The Blast will be processed at some later time this frame; the
   * caller must ensure that the pointer remains valid at least that long.
   *
   * This is more efficient than simply placing the Blast into the GameField
   * and letting it be handled that way, since fewer unneeded physics
   * recalculations must be done.
   *
   * If collideWith() is not currently in the call stack, this call has no
   * significant effect.
   */
  void injectBlastCollision(Blast*) noth;

  virtual const NebulaResistanceElement* getNebulaResistanceElements() noth;
  virtual unsigned getNumNebulaResistanceElements() const noth;

  /** Applies velocity changes to the specified point
   * with the given strength and coordinates of origin.
   * This assumes the strength is correct for that point.
   */
  void applyCollision(float px, float py, float strength, float ox, float oy) noth;

  virtual ~Ship();

  /**
   * Returns the death attributions for the ship as a space-separated
   * list of integers. The list will not be empty, and may have up
   * to four elements (primary, assist, secondary assist 1, secondary assist 2).
   */
  const char* getDeathAttributions() noth;

  /**
   * Returns the damage dealt to this by the given blame;
   * returns 0 if no record exists.
   */
  float getDamageFrom(unsigned) const noth;

  /** Returns the Cell at the given index.
   * Bounds checking is performed.
   */
  Cell* cellAt(unsigned i) const {
    return cells.at(i);
  }

  /** Adds the specified Cell if it is not already inserted. */
  void addCell(Cell* c) noth {
    if (std::find(cells.begin(), cells.end(), c)==cells.end())
      cells.push_back(c);
  }

  /** Removes the specified Cell from the Ship.
   *
   * If it is not present, an std::runtime_error is thrown.
   */
  void removeCell(Cell* c) {
    std::vector<Cell*>::iterator it=std::find(cells.begin(), cells.end(), c);
    if (it != cells.end()) cells.erase(it);
    else throw std::runtime_error("Attempt to remove non-existent cell");
  }

  /** Returns the index of the given cell, or -1 if not found. */
  int findCell(Cell* c) const noth {
    for (unsigned i=0; i<cells.size(); ++i)
      if (cells[i]==c) return (int)i;
    return -1;
  }

  /** Returns the number of cells in the Ship */
  unsigned cellCount() const noth {
    return cells.size();
  }

  /** Returns the category into which this Ship falls. */
  Category categorise() const noth;
  /** Categorises a ship by examining only its config root. */
  static Category categorise(const char*);

  private:
  /* Subtract the given Cell's information from the ship, and perform any
   * other necessary maintenance that must be done before removing it
   * from the ship.
   * This does not actually remove the cell.
   */
  void preremove(Cell*) noth;

  /* Update curr power values according to throttle etc */
  void updateCurrPower() noth;

  //Do the nessesary pre-mortum operations. If the argument is true, the ship
  //is about to be deleted.
  void death(bool) noth;
};

/** Verifies that the given Ship is indeed valid.
 * Returns NULL on success, or a string describing
 * the problem on failure.
 * @param ship The ship to verify
 * @param phys Check physical properties (very expensive)
 */
const char* verify(Ship* ship, bool phys=true);

/**
 * Returns the integer representation of the given category.
 * This is a utility function for Tcl.
 */
static inline unsigned shipCategoryToInt(Ship::Category cat) {
  return (unsigned)cat;
}


#endif /*SHIP_HXX_*/
