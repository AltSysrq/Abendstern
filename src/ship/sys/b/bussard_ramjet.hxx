/**
 * @file
 * @author Jason Lingle
 * @brief Contains the BussardRamjet system
 */

#ifndef BUSSARD_RAMJET_HXX_
#define BUSSARD_RAMJET_HXX_

#include "src/ship/sys/engine.hxx"

/** The BussardRamjet is a device that collects and
 * compresses interstellar hydrogen and helium and
 * causes it to undergo fusion, thereby yielding a
 * high amount of thrust.
 *
 * However, it has a majour
 * limitation in that a front face, less than 30 o
 * off of backT, must be exposed for intake.
 */
class BussardRamjet : public Engine {
  public:
  BussardRamjet(Ship* parent);

  virtual bool acceptCellFace(float) noth;
  virtual unsigned mass() const noth;
  virtual void audio_register() const noth;

  virtual float thrust() const noth;
  DEFAULT_CLONEWO(BussardRamjet)
};

#endif /*BUSSARD_RAMJET_HXX_*/
