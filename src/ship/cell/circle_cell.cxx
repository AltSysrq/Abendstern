/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/cell/circle_cell.hxx
 */

#include <GL/gl.h>
#include <cmath>
#include <cstdlib>

#include "circle_cell.hxx"
#include "src/globals.hxx"
#include "src/graphics/matops.hxx"
using namespace std;

float CircleCell::getIntrinsicDamage() const noth {
  return 13;
}

void CircleCell::drawThis() noth {
  BEGINGP("CircleCell")
  drawSquareBase();
  drawCircleOverlay();
#ifdef AB_OPENGL_14
  drawAccessories();
  mUScale(STD_CELL_SZ/2);
#endif
  drawSystems();
  ENDGP
}
