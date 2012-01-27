/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Objective cortex input provider
 */

/*
 * ci_objective.hxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#ifndef CI_OBJECTIVE_HXX_
#define CI_OBJECTIVE_HXX_

class GameObject;

namespace cortex_input {
  /**
   * Used internally by Objective.
   */
  struct ObjectiveInputOffsets {
    enum t { ox = 0, oy, ot, ovx, ovy, ovt, orad, omass };
  };

  /**
   * Used internally by Objective.
   */
  void objectiveGetInputs(const GameObject*,float*);

  /**
   * Provides coordinate (and, for Ships, rotation, radius,
   * and size) information about a specified objective.
   * Like CellEx, its bindInputs() does not handle this class's
   * inputs -- an object is required, specified in
   * getObjectiveInputs(const GameObject*,float*).
   */
  template<typename Parent>
  class Objective: public Parent {
    protected:
    Objective() {}

    static const unsigned cip_first = Parent::cip_first;
    static const unsigned cip_last = cip_first + 1 + (unsigned)ObjectiveInputOffsets::omass;
    static const unsigned
      cip_ox    = cip_first + (unsigned)ObjectiveInputOffsets::ox,
      cip_oy    = cip_first + (unsigned)ObjectiveInputOffsets::oy,
      cip_ot    = cip_first + (unsigned)ObjectiveInputOffsets::ot,
      cip_ovx   = cip_first + (unsigned)ObjectiveInputOffsets::ovx,
      cip_ovy   = cip_first + (unsigned)ObjectiveInputOffsets::ovy,
      cip_ovt   = cip_first + (unsigned)ObjectiveInputOffsets::ovt,
      cip_orad  = cip_first + (unsigned)ObjectiveInputOffsets::orad,
      cip_omass = cip_first + (unsigned)ObjectiveInputOffsets::omass;
    class cip_input_map: public Parent::cip_input_map {
      public:
      cip_input_map() {
        #define I(i) Parent::cip_input_map::ins(#i, cip_##i)
        I(ox);
        I(oy);
        I(ot);
        I(ovx);
        I(ovy);
        I(ovt);
        I(orad);
        I(omass);
        #undef I
      }
    };

    void getInputs(float* dst) { Parent::getInputs(dst); }
    void getObjectiveInputs(const GameObject* go, float* dst) {
      objectiveGetInputs(go,dst+cip_first);
    }
  };
}

#endif /* CI_OBJECTIVE_HXX_ */
