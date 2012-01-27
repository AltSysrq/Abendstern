/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Weapon cortex input provider.
 */

/*
 * ci_weapon.hxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#ifndef CI_WEAPON_HXX_
#define CI_WEAPON_HXX_

class Launcher;

namespace cortex_input {
  /** Used internally by Weapon */
  void weaponGetInputs(const Launcher*, float*);

  /** Used internally by Weapon */
  struct WeaponInputOffsets {
    enum t { wx, wy, wt, wn };
  };

  /**
   * Provides coordinate and angle offset inputs for Launchers.
   * Like the CellEx, its bindInputs() function does not actually
   * set the inputs for this class; use getWeaponInputs() instead.
   */
  template<typename Parent>
  class Weapon: public Parent {
    protected:
    static const unsigned cip_first = Parent::cip_last;
    static const unsigned cip_last = cip_first + 1 + (unsigned)WeaponInputOffsets::wn;
    static const unsigned cip_wx = cip_first + (unsigned)WeaponInputOffsets::wx,
                          cip_wy = cip_first + (unsigned)WeaponInputOffsets::wy,
                          cip_wt = cip_first + (unsigned)WeaponInputOffsets::wt,
                          cip_wn = cip_first + (unsigned)WeaponInputOffsets::wn;

    class cip_input_map: public Parent::cip_input_map {
      public:
      cip_input_map() {
        #define I(i) Parent::cip_input_map::ins(#i, cip_##i)
        I(wx);
        I(wy);
        I(wt);
        I(wn);
        #undef I
      }
    };

    Weapon() {}
    void bindInputs(float* dst) { Parent::bindInputs(dst); }
    void getWeaponInputs(const Launcher* l, unsigned numLaunchers, float* dst) const {
      weaponGetInputs(l,dst+cip_first);
      dst[cip_wn] = numLaunchers;
    }
  };
}

#endif /* CI_WEAPON_HXX_ */
