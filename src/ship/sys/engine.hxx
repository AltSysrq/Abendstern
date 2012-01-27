/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Engine system type
 */

#ifndef ENGINE_HXX_
#define ENGINE_HXX_
#include <utility>
#include <string>

#include "ship_system.hxx"
#include "src/sim/objdl.hxx"

/**
 * The Engine is a general type of system that uses power to produce thrust.
 */
class Engine: public ShipSystem {
  friend class TclEngine;
  public:
  /** Constructs an Engine with the given parms.
   *
   * @param parent The Ship that contains the system
   * @param tex The system texture to use
   * @param rotmul The rotational multiplier (multiplied by torque to get torque-on-demand)
   */
  Engine(Ship* parent, GLuint tex, float rotmul)
  : ShipSystem(parent, tex, Classification_Engine, ShipSystem::Backward),
  rotationalMultiplier(rotmul)
  {
    mountingCondition = "Programmer forgot to set mounting condition";
  }

  protected:
  /** The power usage at 100% thrust.
   * This must be set by the subclass.
   */
  float powerUsageAmt;
  /** The rotational multiplier to multiply by torque to get torque-on-demand. */
  float rotationalMultiplier;
  /** The string to return if an engine is not or cannot be mounted correctly.
   * This must be set by the subclass.
   */
  std::string mountingCondition;

  /** The direction of propulsion relative to the Ship's. */
  float thrustAngle;
  /** Explosion/LightTrail parameter */
  float expSize,
        /** Explosion/LightTrail parameter */
        expLife,
        /** Explosion/LightTrail parameter */
        expSpeed,
        /** Explosion/LightTrail parameter */
        expDensity,
        /** Explosion/LightTrail parameter */
        maxThrust;
  //The colour is determined with: x=rand*xm+xb
  float rm, ///< Trail colour red slope
        rb, ///< Trail colour red offset
        gm, ///< Trail colour green slope
        gb, ///< Trail colour green offset
        bm, ///< Trail colour blue slope
        bb; ///< Trail colour blue offset

  float backX, ///< X offset where to emit the trail
        backY, ///< Y offset where to emit the trail
        backT; ///< Theta (in CCW degrees) of output

  /** Store container's theta at construccion time.
   *
   *  The original drawing function made the invalid
   * assumption that cell->getT() would be constant.
   * Since it CAN change (ie, ship fragmentation),
   * we need to store the original value here.
   */
  float cellT;

  /** Orientation refers to the face index of output. */
  int orientation;

  /** Pointer to our trail */
  ObjDL trail;

  public:
  /** Returns thrustAngle */
  inline float getThrustAngle() noth { return thrustAngle; }
  /** Returns the rotational multiplier.
   * @see rotationalMultiplier
   */
  float getRotationalMultiplier() noth { return rotationalMultiplier; }

  virtual const char* autoOrient() noth;
  virtual const char* setOrientation(int) noth;
  virtual int         getOrientation() const noth;

  virtual signed normalPowerUse() const noth { return powerUsageAmt; }

  /** Determines whether a given intake angle makes the cell acceptable.
   * Default returns true.
   */
  virtual bool acceptCellFace(float theta) noth { return true; }
  /** Returns whether the engine is happy with no intakes.
   * Default returns false.
   */
  virtual bool needsNoIntakes() noth { return false; }
  /** Returns the thrust provided at 100% */
  virtual float thrust() const noth = 0;

  /** Performs Engine-specific updating */
  static void update(Ship*, float) noth;
};

#endif /*ENGINE_HXX_*/
