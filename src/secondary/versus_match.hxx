#ifndef VERSUS_MATCH_HXX_
#define VERSUS_MATCH_HXX_

#include <vector>
#include <string>
#include <exception>
#include <stdexcept>

#include "src/sim/game_field.hxx"
#include "src/core/aobject.hxx"

class Ship;

/**
 * Orchestrates a "match" between two small teams of idential ships, as defined
 * by the ship-match Abendstern network job (see abserver.txt in the
 * abendstern-server repository).
 */
class VersusMatch: public AObject {
  GameField field;
  std::vector<Ship*> testing, against;
  unsigned currentStep;

  VersusMatch(const std::string&, unsigned, const std::string&, unsigned)
  throw (std::runtime_error);

  //Not defined
  VersusMatch(const VersusMatch&);

public:
  /**
   * Tries to create a VersusMatch pitting testcnt ships loaded from conf[test]
   * against againstcnt ships loaded from against[test].
   *
   * On success, returns a non-NULL VersusMatch*. On failure, throws a
   * runtime_error.
   *
   * @param test The config root of the ship to test
   * @param testcnt The number of ships being tested
   * @param against The config root of the ship to test against
   * @param againstcnt The number of enemy ships
   */
  static VersusMatch* create(const std::string& test, unsigned testcnt,
                             const std::string& against, unsigned againstcnt)
  throw (std::runtime_error);

  /**
   * Advances the simulation one step.
   *
   * Returns true if the simulation is not complete.
   */
  bool step() noth;
  /**
   * Returns the score of the test ship, assuming that the match is complete.
   */
  float score() const noth;

  virtual ~VersusMatch();

private:
  void shipDeath(Ship*) noth;
  static void notifyShipDeath(Ship*, bool);
};

#endif /* VERSUS_MATCH_HXX_ */
