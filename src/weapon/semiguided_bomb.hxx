/**
 * @file
 * @author Jason Lingle
 * @brief Contains the SemiguidedBomb weapon
 */

#ifndef SEMIGUIDED_BOMB_HXX_
#define SEMIGUIDED_BOMB_HXX_

#include "magneto_bomb.hxx"
#include "src/sim/objdl.hxx"
#include "src/ship/ship.hxx" //We do need to include this so the compiler knows that Ship extends GameObject

/* Bound in ship/semiguided_bomb_launcher.cxx */
extern const float semiguidedBombSpeed;

#define SEMIGUIDED_SUBMUL 1.2f ///< Value to pass as submul in MagnetoBomb for SemiguidedBomb

/** The SemiguidedBomb is an extension of the MagnetoBomb that
 * is guided by its parent ship. The strength of guidance is
 * determined by the distance between the parent and the target.
 */
class SemiguidedBomb : public MagnetoBomb {
  private:
  //It is not suitable to use MagnetoBomb::parent for this purpose
  ObjDL parent;

  public:
  /** Constructs a SemiguidedBomb with the given parms.
   *
   * @param field Field in which the bomb will live
   * @param x Initial X coordinate
   * @param y Initial Y coordinate
   * @param vx Initial X velocity
   * @param vy Initial Y velocity
   * @param power Energy level of bomb
   * @param par Ship that launched the bomb
   */
  SemiguidedBomb(GameField* field, float x, float y, float vx, float vy, float power, Ship* par)
  : MagnetoBomb(field, x, y, vx, vy, power, par, SEMIGUIDED_SUBMUL, 0.7f, 0.85f, 1.0f),
    parent(par)
  { }

  virtual bool update(float) noth;
};

#endif /*SEMIGUIDED_BOMB_HXX_*/
