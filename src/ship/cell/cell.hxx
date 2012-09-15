/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Cell class and common cell definitions
 */

#ifndef CELL_HXX_
#define CELL_HXX_

#include <set>
#include <vector>
#include <string>
//for GLuint
#include <GL/gl.h>


#include "src/ship/sys/ship_system.hxx"
#include "src/sim/collision.hxx"
#include "src/core/aobject.hxx"
#include "src/opto_flags.hxx"
#include "src/ship/physics_bits.hxx"

class Ship;

/** The "size" of one cell */
#define STD_CELL_SZ (16/1024.0f)
/** A "pixel" unit */
#define PIX (STD_CELL_SZ/16)
#ifndef AB_OPENGL_14
/**
 * Returns true if the given index should be light.
 * COLOURE is for edges, COLOURC for corners.
 * COLOURC_PREP must be included in the function above
 * any COLOURC usages.
 * These macros actually return exactly 0 or 1 to guarantee
 * bitshifting works as expected.
 */
#define COLOURE(n) (neighbours[n]?1:0)
/** @brief See COLOURE. @see COLOURE */
#define COLOURC_PREP int indices[4]; \
                     for (int i=0; i<4; ++i) if (neighbours[i]) { \
                       indices[i]=neighbours[i]->getNeighbour(this); }
/** @brief See COLOURE. @see COLOURE */
//If n (the earlier neighbour's) 's previous relative to us, or o (later) 's next, consider the corner used
#define COLOURC(n,o) ((\
   (neighbours[n] && neighbours[o]) \
|| (neighbours[o] && neighbours[o]->neighbours[(indices[o]+1)%neighbours[o]->numNeighbours()]) \
|| (neighbours[n] && neighbours[n]->neighbours[indices[n]? indices[n]-1 : neighbours[n]->numNeighbours()-1]))\
  ?1:0)
#else
//Old OpenGL 1.1--2.1 definitions
#define COLOURE(n) parent->glSetColour(neighbours[n]? 1.0f : 0.4f)
#define COLOURC_PREP int indices[4]; \
                     for (int i=0; i<4; ++i) if (neighbours[i]) { \
                       indices[i]=neighbours[i]->getNeighbour(this); }
//If n (the earlier neighbour's) 's previous relative to us, or o (later) 's next, consider the corner used
#define COLOURC(n,o) { \
  bool used=false; \
  if (neighbours[n] && neighbours[o]) used=true; \
  else if (neighbours[o] && neighbours[o]->neighbours[(indices[o]+1)%neighbours[o]->numNeighbours()]) used=true; \
  else if (neighbours[n] && neighbours[n]->neighbours[indices[n]? indices[n]-1 : neighbours[n]->numNeighbours()-1]) used=true; \
  parent->glSetColour(used? 1.0f : 0.4f); }
#endif /* AB_OPENGL_14 */

/** Common colours used while drawing Cells. */
namespace cell_colours {
  /** Semitransparent black */
  extern float semiblack[4];
  /** White */
  extern float white[4];
};

/** Defines the usage of a Cell. */
enum CellUsage {
  /** Usage is ship bridge.
   * Every ship must have exactly one bridge.
   * The bridge represents the essence of the ship;
   * if it is destroyed, the entire ship is.
   */
  CellBridge,
  /** Usage is standard (empty or holding ShipSystems).
   * A square or circular cell can hold two systems,
   * a triangular one one system.
   */
  CellSystems
};

/**
 * The Cell is one of the basic units of construction of Ships.
 * This class defines the general interface for the five types of Cells that exist.
 */
class Cell: public AObject {
  friend class Ship;
  //If I don't have this, G++ gives me things like
  //  ../src/ship/cell/cell.hxx:223:9: error: 'float Cell::x' is protected
  //  ../src/ship/cell/empty_cell.cxx:9:18: error: within this context
  //Wtf?
  friend class EmptyCell;
  public:
  /** Records the name of the cell entry from
   * the ship file. This is only useful for
   * the ship editor.
   */
  std::string cellName;

  /** For exclusive use by the networking protocol.
   * Records the original index within the Ship of
   * the Cell.
   */
  unsigned netIndex;

  /** The neighbours of this Cell */
  Cell* neighbours[4];
  /** The Ship to which this Cell belongs. */
  Ship* parent;
  /** The ShipSystems contained by this Cell.
   * Depending on type, Cells may contain 1 or 2
   * (or 0 in the case of EmptyCell).
   * @see usage
   */
  ShipSystem* systems[2];
  /** The usage of this Cell.
   * @see CellUsage
   */
  CellUsage usage;

  /** Represents the amount of damage taken to this Cell.
   * @see getMaxDamage()
   */
  float damage;

  private:
  //A GL texture representing damage. This is ARGB with
  //all pixels black, but initially with 0 alpha. At
  //100% damage, the average alpha will be 0.5
  GLuint damageTexture;

  unsigned char damageTextureData[16*16*4];

  public:
  /** Physical information used for determining the ship's
   * physics.
   */
  struct PhysicalInformation {
    /** Bitset of ready values. */
    physics_bits valid;

    /** The calculated mass of the cell.<br>
     * Invalidated by: deletion of contained system<br>
     * Depends on: Nothing <br>
     * Affects: rotational inertia, centre of gravity <br>
     * Provides: mass
     */
    float mass;
    /** Angle from the bridge to this cell, as well as the cosine
     * and sine of that angle and the length of this vector.
     * This is associated with the more global location (Cell::x
     * and Cell::y). <br>
     * Invalidated by: coordinate change <br>
     * Depends on: Nothing <br>
     * Affects: rotational inertia, centre of gravity, torque <br>
     * Provides: Nothing
     */
    float angle, /**@see angle*/ cosine, /**@see angle*/ sine,
          /**@see angle*/ distance;
    /** Contributions to thrust produced by engines in this cell, when
     * the throttle is 100%. <br>
     * Invalidated by: Change of stealth mode <br>
     * Depends on: Nothing <br>
     * Affects: torque <br>
     * Provides: Thrust magnitude, thrust direction
     */
    float thrustX, /**@see thrustX*/ thrustY;
    /** Contributions to torque produced by engines in this cell.
     * If the Cell* is non-NULL, the torque is nearly-exactly counter-
     * balanced by that cell, and the combined torque by both is
     * considered to be exactly zero. <br>
     * Invalidated by: coordinate change, loss of linked cell, change of
     * stealth mode <br>
     * Provides: total torque
     */
    float torque;
    /** @see torque */
    Cell* torquePair;
    /** Contributions to rotational torque by this cell.
     * That is, desired rotation (ie, controlled by the actual
     * controller and not due to thrust imbalance). <br>
     * Invalidated by: coordinate change, change of stealth mode <br>
     * Provides: rotational thrust
     */
    float rotationalThrust;
    /** Contributions to cooling by this cell.
     * This is a multiplier; when the cell is removed, this value
     * can simply be divided into the ship's. <br>
     * Invalidated by: Nothing <br>
     * Affects: Nothing <br>
     * Provides: Cooling
     */
    float cooling;
    /** Contributions to the power system by this cell.
     * The four values indicate the four possible types of consumption
     * (whether it happens in stealth mode, and whether it is multiplied
     *  by throttle; these are U/S (unstealth/stealth) and C/T (constant/throttled)). <br>
     * Invalidated by: System destruction <br>
     * Affects: Nothing <br>
     * Provides: Power usage
     */
    signed powerUC, /**@see powerUC*/ powerUT, /**@see powerUC*/ powerSC, /**@see powerUC */ powerST;
    /** Only negative contributions of the above; none depend on throttle.
     * This only includes power generators. <br>
     * Invalidated by: System destruction <br>
     * Affects: Nothing <br>
     * Provides: Power production
     */
    unsigned ppowerU, /**@see ppowerU */ ppowerS;
    /** Number of heat producers in the cell.<br>
     * Invalidated by: Nothing <br>
     * Affects: Global cooling rate <br>
     * Provides: Heating count
     */
    unsigned numHeaters;
    /** Contributions to capacitance by this cell.
     * When the cell is destroyed, this can simply be subtracted from
     * the ship's value. <br>
     * Invalidated by: Nothing <br>
     * Affects: Nothing <br>
     * Provides: Total capacitance
     */
    unsigned capacitance;
    /** Set to true if the cell contains a working dispersion shield.
     * <this> is the first cell in the linked list of dependencies
     * on the dispersion shield. <br>
     * Invalidated by: Nothing <br>
     * Affects: Dispersion shielding <br>
     * Provides: Nothing <br>
     */
    bool hasDispersionShield;
    /** Stores the nearest Cell with dispersion shielding, as well as the
     * distance to it, and the next Cell that depends on that Cell. <br>
     * Invalidated by: Destruction of cell with dispersion shielding. <br>
     * Affects: Nothing <br>
     * Provides: Dispersion shielding
     */
    Cell* nearestDS;
    /** @see nearestDS */
    Cell* nextDepDS;
    /** @see nearestDS */
    Cell* prevDepDS;
    /** @see nearestDS */
    float distanceDS;
    /** Stores the internal reinforcement of this cell. <br>
     * Invalidated by: Nothing <br>
     * Affects: Nothing <br>
     * Provides: internal reinforcement
     */
    float reinforcement;
  };
  protected:
  /** Physical information of this Cell */
  PhysicalInformation physics;

  public:
  /** True for SquareCells and CircleCells.
   * Must be set by SquareCell.
   */
  bool isSquare;
  /** True for EmptyCells.
   * Must be set by EmptyCell.
   */
  bool isEmpty;

  protected:

  float
    /** X component of the vector from the centre of gravity to this Cell */
    x,
    /** Y component of the vector from the centre of gravity to this Cell */
    y;
  /** Rotation in degrees (CCW) */
  int theta;
  /** Coordinate information is only valid if this is true. */
  bool oriented;

  /** Generally returned by getCollisionBounds(). */
  CollisionRectangle collisionRectangle,
    /** Returned by getCollisionBoundsBeta() */
    collisionRectangleBeta;

  /** Initialises a Cell for use with the given Ship. */
  Cell(Ship*);

  /** The base mass of the Cell itself at reinforcement zero.
   * This MUST be set by subclasses.
   */
  unsigned _intrinsicMass;

  public:
  /** Returns the index of the given neighbour, or abort if not found */
  unsigned getNeighbour(const Cell* other) noth;
  /** Returns how many neighbours are supported */
  virtual unsigned numNeighbours() const noth = 0;
  /** Returns the relative distance between this cell's centre
   * and the edge to which the given neighbour attaches.
   */
  virtual float edgeD(int n) const noth = 0;
  /** Return the angle in CCW degrees to the given edge. */
  virtual signed edgeT(int n) const noth = 0;
  /** Returns the rotation in CCW degrees incurred by attaching to the given
   * edge. This is to be 0..89 (actually, +/- 0|30|45).
   * By default, return 0, which works for squares and circles.
   */
  virtual signed edgeDT(int n) const noth { return 0; }
  /** Returns the rotation in CCW degrees incurred upon this cell when it
   * has a neighbour attached to the given side.
   * Default returns 0.
   */
  virtual signed edgeIDT(int n) const noth { return 0; }

  /** Returns the intrinsic mass of the cell type */
  unsigned intrinsicMass() const noth { return _intrinsicMass; };

  /** Returns the current state of the internal physical information.
   * No initial calculations are performed.
   */
  const PhysicalInformation& getPhysics() const noth { return physics; }
  /** Returns the state of the physical information after ensuring
   * that the specified bits are known.
   */
  const PhysicalInformation& getPhysics(physics_bits pb) noth {
    physicsRequire(pb);
    return physics;
  }

  /** Returns the amount of damage the cell can take */
  float getMaxDamage() const noth;
  /** Returns the intrinsic damage the cell type can take */
  virtual float getIntrinsicDamage() const noth = 0;
  /** Returns the current damage ammount */
  float getCurrDamage() const noth;
  /** Damages the cell by the given amount and blame.
   * Returns whether the cell is still intact.
   * If the cell is damaged below its intrinsic amount,
   * the damage() method of both systems is called, which
   * can result in a Blast (this is only done if not remote).
   */
  bool applyDamage(float amount, unsigned blame) noth;

  /** Returns a CollisionRectangle that represents
   * the boundaries of this Cell.
   * This is not implemented by default because certain
   * parms must be set based on the current frame.
   * @return The collision bounds, or NULL for EmptyCells
   */
  virtual CollisionRectangle* getCollisionBounds() noth = 0;

  /** Makes this Cell the root cell and orients all
   * others according to it.
   */
  void orient(int initTheta=0) noth;
  /** Orient all neighbours according to this cell's current orientation.
   * Assumes this cell has already been properly oriented.
   */
  void orientImpl() noth;

  /** Mark the cell as needing orientation.
   * All cells should be marked this way before
   * calling orient(), except for the first time.
   */
  void disorient() noth { oriented=false; }

  /** Returns x. @see x */
  inline float getX() const noth { return x; }
  /** Returns y. @see y */
  inline float getY() const noth { return y; }
  /** Returns theta. @see theta */
  inline float getT() const noth { return theta; }

  /** Returns the X offset of this Cell's centre of gravity from its logical
   * centre, assuming zero rotation.
   */
  virtual float getCentreX() const noth { return 0; }
  /** Returns the Y offset of this Cell's centre of gravity from its logical
   * centre, assuming zero rotation.
   */
  virtual float getCentreY() const noth { return 0; }

  /** Draws the cell. If translate is true, apply rotation and translation
   * before drawing.
   */
  void draw(bool translate=true) noth;

  /** Draw the shape, coloured according to damage. */
  void drawShape(const float* healthyColour, const float* damagedColour) noth;

  /** Draw the damage graphic for the cell. */
  void drawDamage() noth;

  /** Called by Ship::update if the ship has been invisible for several
   * seconds. This funciton deletes the damage texture of the cell so
   * that video memory is freed.
   */
  void freeTexture() noth;

  /** Returns a vector of Cells that are all joined in some way to this Cell.
   * Returns immediately if the Cell is already in the vector.
   */
  void getAdjoined(std::set<Cell*>&) noth;

  /** Clears the specified physics flags for this cell.
   * Notifies the parent ship that some cells do not have valid
   * values for these bits. Any ship-global bits are passed
   * to the ship as well.
   * This does NOT perform any unlinking in the case of dispersion
   * shielding, etc.
   */
  inline void physicsClear(physics_bits pb) noth {
    physics.valid &= ~pb;
    parent->physicsClear(pb);
  }

  /** Ensures the specified physical values are valid before
   * returning.
   *
   * Note that requiring PHYS_CELL_DS_EXIST_BIT when there
   * really is a dispersion shield present will invalidate
   * any cells already known to be nearest this WITHOUT
   * CLEARING THE RELEVANT INFORMATION. Any code that clears
   * PHYS_CELL_DS_EXIST_BIT MUST, therefore, also clear
   * PHYS_CELL_DS_NEAREST_BIT on EVERY cell in the ship.
   */
  void physicsRequire(physics_bits) noth;

  /** Properly destroys information in the chain regarding
   * a DispersionShield in this Cell.
   *
   * This should be done prior to calling physicsClear()
   * with PHYS_CELL_DS_EXIST_BITS, if the calling code can't
   * do it itself.
   */
  void clearDSChain() noth;

  virtual ~Cell();
  protected:
  /** Provided by the subclass to perform actual drawing. */
  virtual void drawThis() noth = 0;

  /** Provided by the subclass to perform actual drawing. */
  virtual void drawShapeThis(float r, float g, float b, float a) noth = 0;

  /** Draw the damage texture on the cell. The texture has already
   * been set, as well as standard transforms.
   */
  virtual void drawDamageThis() noth = 0;

  private:
  /** Same as getCollisionBounds(), but copies value into collisionRectangleBeta
   * and returns that.
   * This ensures that it will remain constant for collision detection.
   */
  CollisionRectangle* getCollisionBoundsBeta() noth {
    if (!getCollisionBounds()) return NULL;
    collisionRectangleBeta = collisionRectangle;
    return &collisionRectangleBeta;
  }
};
#endif /*CELL_HXX_*/
