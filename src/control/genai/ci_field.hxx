/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Field cortex input provider.
 */
/*
 * ci_field.hxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#ifndef CI_FIELD_HXX_
#define CI_FIELD_HXX_

#include <map>
#include <string>

#include "src/opto_flags.hxx"

class GameField;

namespace cortex_input {
/** Internally used by Field. */
void bindFieldInputs(float*, const GameField*);

/**
 * Provides the fieldw and fieldh inputs.
 * The setField(GameField*) must be called before this class will work.
 */
template<typename Parent>
class Field: public Parent {
  const GameField* field;

  public:
  static const unsigned cip_first = Parent::cip_last;
  static const unsigned cip_last = cip_first+2;
  static const unsigned cip_fieldw = cip_first;
  static const unsigned cip_fieldh = cip_first+1;

  protected:
  Field() {}
  /**
   * Sets the field to operate on. This must be called before
   * the class is operational.
   */
  void setField(GameField* fld) { field = fld; }

  class cip_input_map: public Parent::cip_input_map {
    public:
    cip_input_map() {
      Parent::cip_input_map::ins("fieldw", cip_fieldw);
      Parent::cip_input_map::ins("fieldh", cip_fieldh);
    }
  };

  void bindInputs(float* inputs) noth {
    Parent::bindInputs(inputs);
    bindFieldInputs(inputs+cip_first, field);
  }
};
}

#endif /* CI_FIELD_HXX_ */
