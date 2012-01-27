/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_field.hxx
 */

/*
 * ci_field.cxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#include "src/sim/game_field.hxx"
#include "ci_field.hxx"

void cortex_input::bindFieldInputs(float* dst, const GameField* field) {
  dst[0] = field->width;
  dst[1] = field->height;
}
