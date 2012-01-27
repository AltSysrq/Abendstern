/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/mat.hxx
 */

/*
 * mat.cxx
 *
 *  Created on: 22.01.2011
 *      Author: jason
 */

#include <iostream>
#include <cmath>
#include <algorithm>
#include "mat.hxx"

using namespace std;

std::ostream& operator<<(std::ostream& out, const matrix& m) {
  for (unsigned r=0; r<4; ++r) {
    for (unsigned c=0; c<4; ++c)
      out << m(r,c) << '\t';
    out << std::endl;
  }
  out << std::endl;
  return out;
}

matrix matrix::operator!() const {
  /* Algorithm for in-place matrix inversion adapted
   * from here:
   * http://www.geometrictools.com/LibMathematics/Algebra/Wm5Matrix4.inl
   *
   * I have absolutely no idea how this works, but since we accidentally
   * stored our matrices sanely (instead of the OpenGL sideways manner),
   * it should all work out anyway.
   */
  float a0 = v[ 0]*v[ 5] - v[ 1]*v[ 4],
        a1 = v[ 0]*v[ 6] - v[ 2]*v[ 4],
        a2 = v[ 0]*v[ 7] - v[ 3]*v[ 4],
        a3 = v[ 1]*v[ 6] - v[ 2]*v[ 5],
        a4 = v[ 1]*v[ 7] - v[ 3]*v[ 5],
        a5 = v[ 2]*v[ 7] - v[ 3]*v[ 6],
        b0 = v[ 8]*v[13] - v[ 9]*v[12],
        b1 = v[ 8]*v[14] - v[10]*v[12],
        b2 = v[ 8]*v[15] - v[11]*v[12],
        b3 = v[ 9]*v[14] - v[10]*v[13],
        b4 = v[ 9]*v[15] - v[11]*v[13],
        b5 = v[10]*v[15] - v[11]*v[14];
  float det = a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0;
  float idet = 1/det;

  return matrix(
      (+ v[ 5]*b5 - v[ 6]*b4 + v[ 7]*b3)*idet,
      (- v[ 1]*b5 - v[ 2]*b4 + v[ 3]*b3)*idet,
      (+ v[13]*a5 - v[14]*a4 + v[15]*a3)*idet,
      (- v[ 9]*a5 + v[10]*a4 - v[11]*a3)*idet,
      (- v[ 4]*b5 + v[ 6]*b2 - v[ 7]*b1)*idet,
      (+ v[ 0]*b5 - v[ 2]*b2 + v[ 3]*b1)*idet,
      (- v[12]*a5 + v[14]*a2 - v[15]*a1)*idet,
      (+ v[ 8]*a5 - v[10]*a2 + v[11]*a1)*idet,
      (+ v[ 4]*b4 - v[ 5]*b2 + v[ 7]*b0)*idet,
      (- v[ 0]*b4 + v[ 1]*b2 - v[ 3]*b0)*idet,
      (+ v[12]*a4 - v[13]*a2 + v[15]*a0)*idet,
      (- v[ 8]*a4 + v[ 9]*a2 - v[11]*a0)*idet,
      (- v[ 4]*b3 + v[ 5]*b1 - v[ 6]*b0)*idet,
      (+ v[ 0]*b3 - v[ 1]*b1 + v[ 2]*b0)*idet,
      (- v[12]*a3 + v[13]*a1 - v[14]*a0)*idet,
      (+ v[ 8]*a3 - v[ 9]*a1 + v[10]*a0)*idet);
}
