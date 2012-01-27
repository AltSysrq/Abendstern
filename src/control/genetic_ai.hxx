/**
 * @file
 * @author Jason Lingle
 * @brief Contains the GeneticAI class and utility functions
 */

/*
 * genetic_ai.hxx
 *
 *  Created on: 16.10.2011
 *      Author: jason
 */

#ifndef GENETIC_AI_HXX_
#define GENETIC_AI_HXX_

#include <vector>
#include <map>

#include "controller.hxx"

struct ObjDL;

namespace libconfig {class Setting;}

/**
 * The GeneticAI class interprets genetic AI data according to the
 * description in ai_design.txt in the root of this project.
 *
 * When instantiated, the GeneticAI picks a random species/instance
 * combination from those compiled in (see compileGeneticAI()).
 *
 * The class assumes that the ai list information is mounted in genai,
 * and that each AI species is under ai:NAME.
 */
class GeneticAI: public Controller {
  public:
  ///input/output constants (used internally)
  enum InputOutput {
    IORetarget = 0,
    IOScantgt,
    IOSx, IOSy, IOSvx, IOSvy, IOSt, IOSvt,
    IOTx, IOTy, IOTvx, IOTvy, IOTt, IOTvt, IOTid,
    IOFx, IOFy, IOFvx, IOFvy, IOFt, IOFvt, IOFid,
    IOFieldw, IOFieldh, IOTime, IORand, IONoise,
    IOSwarmcx, IOSwarmcy, IOSwarmvx, IOSwarmvy,
    IOPaintt, IOPainta, IOPainpt, IOPainpa,
    IOHappys, IOHappyg, IOSads, IOSadg,
    IONervous, IOFear, IOFrustrate, IODislike,
    IOFocusMode, IOFocusx, IOFocusy,
    IOF2nf, IOF2ne,
    IOF1concern,
    IOFtype,
    IOF0celld,
    IOF0sys0t, IOF0sys0c, IOF0sys1t, IOF0sys1c,
    IOF0bridge,
    IOSmass, IOSacc, IOSrota, IOSradius,
    IOSpoweru, IOSpowerp, IOScapac, IOShasweap,
    IOSwx, IOSwy, IOSwt,
    IOThrottle, IOAccel, IOBrake, IOTurning,
    IOStealth, IOWeapon, IOWeaplvl, IOFire, IOSelfdestr,
    IOMem0, IOMem1, IOMem2, IOMem3,
    IOMem4, IOMem5, IOMem6, IOMem7,
    IOTel0, IOTel1, IOTel2, IOTel3,
    IOTel4, IOTel5, IOTel6, IOTel7
  };
  ///The appropriate array size to hold all I/O values
  static const unsigned ioLength = ((unsigned)IOTel7)+1;
  ///The number of telepathic I/O values (which are always at the end of the enum)
  static const unsigned numTelepathy = 8;

  ///Function to perform a step of evaluation.
  ///  stack     Current evaluation stack; increment to push, decrement to pop
  ///  fparm     Floating-point parameters; increment after using one
  ///  iparm     Integer parameters; increment after using one
  ///  cost      Cumulative cost of evaluation (add to it after evaluation)
  ///  next      The evalf array, pointing to the next item to execute
  ///            (this must be void* since function types cannot be recursive).
  ///            This is only to be used by short-circuiting functions that must
  ///            alter flow
  typedef void (*evaluator_fun)(float*& stack, const float*& fparm, const unsigned*& iparm,
                                unsigned& cost, const void*const*& next);

  private:
  //Current values for all normal I/O values
  //(this array includes a number of slots that won't be used because
  // they are not normal, such as noise and pure outputs.)
  //The last numTelepathy slots are parallel to linkedFriends.
  float basicInputs[ioLength];

  //Stores the GeneticAIs that have telepathic links to us (bidirectional)
  GeneticAI* linkedFriends[numTelepathy];
  //Addresses to write telepathic output to; this array is parallel to
  //linkedFriends
  float* telepathicOutputs[numTelepathy];

  //Arrays of all evalfs, fparms, and iparms
  std::vector<evaluator_fun> allEvalfs;
  std::vector<float> allFParms;
  std::vector<unsigned> allIParms;

  //entry point for each output;
  //entries are indices instead of pointers so that
  //the vectors can resize after the data is set.
  struct entry_point {
    unsigned eval;
    unsigned fparm;
    unsigned iparm;
    unsigned length; //Number of evalfs to execute
  } outputs[ioLength];

  //For the persistent pain, keep track of total pain in eight octants,
  //and use the maximum for the actual input.
  //Divided by (minimum) and reported direction:
  //  0 -π1     -7π/8
  //  1 -3pi/4  -5π/8
  //  2 -π/2    -3π/8
  //  3 -π/4    -1π/8
  //  4 0       +1π/8
  //  5 +π/4    +3π/8
  //  6 +π/2    +5π/8
  //  7 +3π/4   +7π/8
  float cumulativeDamage[8];

  //Reference into Ship's target
  ObjDL& target;
  float timeSinceRetarget;

  //Keeps track of cost ticks; this is reduced every update, and
  //actual updating is performed when it is <= 0
  signed ticksUntilUpdate;
  //Milliseconds since previous primary update
  float timeSinceUpdate;

  //Time until the next update of "expensive" information
  //(eg, fear, nervous, finding telepathic friends).
  //Every update, this is set to a random time between
  //1024 and 2048 milliseconds.
  float timeUntilExpensiveUpdate;

  //Keep track of integer IDs for ships.
  //Used IDs are mapped into shipids; freed (past) shipids
  //are placed into freeShipids until they are used again;
  //nextShipID keeps track of the next integer to use when
  //nothing is free.
  std::map<Ship*,signed> shipids;
  std::vector<signed> freeShipids;
  signed nextShipID;

  //Store the last known ship insignia; if this changes,
  //remove from map and reinsert
  unsigned long lastInsignia;

  public:
  /**
   * If true, the constructor failed and this is not a valid GeniticAI.
   */
  bool failed;
  unsigned species, ///< The species index of this AI
           generation, ///< The generation of the species
           instance; ///< The instance within the species

  /**
   * Constructs a new GeneticAI on the given Ship.
   *
   * The species and instance are chosen randomly from what is available
   * in the latest generations.
   *
   * This may throw unspecified exceptions if the AI information is invalid;
   * if this occurs, the caller must attach some other controller to the ship.
   *
   * The constructor never throws an exception; if it fails, the failed
   * variable becomes true.
   */
  GeneticAI(Ship*);
  virtual ~GeneticAI();

  virtual void update(float) noth;
  virtual void damage(float,float,float) noth;
  virtual void otherShipDied(Ship*) noth;
  virtual void notifyScore(signed) noth;

  private:
  //Compiles the given Output with the given name;
  //if unsuccessful, the expression is constant 0 and a warning
  //is printed to stderr
  void compileOutput(InputOutput, const char*) noth;
  //Notifies this ai that another is terminiting its link with
  //us; this is only valid if linked.
  //It assumes this is due to destruction of that friend, and
  //alters sads appropriately.
  void disconnectTepepathy(GeneticAI*) noth;
  //Requests to open a link with the given ai, using the given
  //float* for output.
  //This is legal if:
  //  This has at least one free link
  //  The other one is not this
  //  There is already a link
  //If legal, the link is establashed with this.
  //Returns the link output if legal, NULL otherwise.
  float* acceptTelepathy(GeneticAI*, float*) noth;
  //Returns the integer ID for the given Ship, allocating
  //a new ID if necessary
  signed getShipID(Ship*) noth;
  //Evaluates the given Output and returns the result
  float eval(InputOutput) noth;
};

/**
 * Determines cost (with clock()) of a number of functions and dumps the
 * report to stdout. The total time for running each function 1000 times
 * is used.
 *
 * This is only useful to determine the appropriate costs of those functions
 * during evaluation.
 */
void calculateGeneticAIFunctionCosts();

#endif /* GENETIC_AI_HXX_ */
