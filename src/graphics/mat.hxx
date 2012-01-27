/**
 * @file
 * @author Jason Lingle
 * @brief Contains the matrix class and functions to operate on it.
 * 4x4 matrix operations.
 *
 * (I really don't see any reason to support
 * other matrix types). Includes a matrix stack.
 */

/*
 * mat.hxx
 *
 *  Created on: 20.01.2011
 *      Author: jason
 */

 #ifndef MAT_HXX_
#define MAT_HXX_

#include <cstring>
#include <iostream>
#include "vec.hxx"

/** The float array representing the identity matrix. */
static const float identityMatrix[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

/**
 * Contains the data and operators for working with matrices.
 * This is NOT a POD type.
 *
 * Matrices were intended to be stored column-major, as OpenGL does.
 * Due to error, they are stored in sane C++-style row-major order,
 * though, and must be transposed before being handed to OpenGL.
 *
 * \verbatim
 * Operators:
 *   +, -       Matrix addition
 *   *          Scalar multiplication
 *   *          Matrix multiplication
 *   *          Vector multiplication
 *   ~          Transpose
 *   !          Inverse
 * \endverbatim
 */
class matrix {
  static const unsigned len=16;
  public:
  /** The values in the matrix, in row-major order */
  float v[16];

  /** Returns the value at the specified index */
  float operator[](unsigned i) const {
    return v[i];
  }
  /** Returns the value at the specified index */
  float& operator[](unsigned i) {
    return v[i];
  }

  /** Returns the value in the specified row,column */
  inline float operator()(unsigned r,unsigned c) const {
    return v[(r<<2) + c];
  }
  /** Returns the value in the specified row,column */
  inline float& operator()(unsigned r,unsigned c) {
    return v[(r<<2) + c];
  }

  /** Constructs an identity matrix */
  matrix() {
    std::memcpy(v, identityMatrix, sizeof(identityMatrix));
  }

  /** Constructs a matrix with the specified values */
  matrix(float f00, float f01, float f02, float f03,
         float f10, float f11, float f12, float f13,
         float f20, float f21, float f22, float f23,
         float f30, float f31, float f32, float f33) {
    /**\cond INTERNAL*/
    #define A(r,c) (*this)(r,c)=f##r##c
    A(0,0); A(0,1); A(0,2); A(0,3);
    A(1,0); A(1,1); A(1,2); A(1,3);
    A(2,0); A(2,1); A(2,2); A(2,3);
    A(3,0); A(3,1); A(3,2); A(3,3);
    #undef A
    /**\endcond*/
  }

  /** Constructs a matrix which is a copy of the other */
  matrix(const matrix& m) {
    std::memcpy(v, m.v, sizeof(v));
  }

  /** Copies a matrix with the given initial contents of v */
  matrix(const float* f) {
    std::memcpy(v, f, sizeof(v));
  }

  /** Assigns this matrix to the other's contents */
  matrix& operator=(const matrix& o) {
    std::memcpy(v, o.v, sizeof(v));
    return *this;
  }

  /** Assigns this matrix's contents to those provided */
  matrix& operator=(const float* f) {
    std::memcpy(v,f,sizeof(v));
    return *this;
  }

  matrix operator+(const matrix& m) const {
    matrix ret(*this);
    ret += m;
    return ret;
  }

  matrix& operator+=(const matrix& m) {
    for (unsigned i=0; i<len; ++i)
      v[i]+=m[i];
    return *this;
  }

  matrix operator-(const matrix& m) const {
    matrix ret(*this);
    ret -= m;
    return ret;
  }

  matrix& operator-=(const matrix& m) {
    for (unsigned i=0; i<len; ++i)
      v[i]+=m[i];
    return *this;
  }

  matrix operator*(float f) const {
    matrix ret(*this);
    ret *= f;
    return ret;
  }

  matrix& operator*=(float f) {
    for (unsigned i=0; i<len; ++i)
      v[i]*=f;
    return *this;
  }

  matrix operator*(const matrix& m) const {
    const matrix& t(*this);
    matrix r;
    /**\cond INTERNAL*/
    #define MUL(x,y) r(x,y) = t(0,y)*m(x,0) + t(1,y)*m(x,1) + t(2,y)*m(x,2) + t(3,y)*m(x,3)
    #define MUL4(x) MUL(x,0); MUL(x,1); MUL(x,2); MUL(x,3)
    MUL4(0);
    MUL4(1);
    MUL4(2);
    MUL4(3);
    #undef MUL4
    #undef MUL
    /**\endcond*/
    return r;
  }

  matrix& operator*=(const matrix& m) {
    return *this = (*this)*m;
  }

  vec4 operator*(const vec4& v) const {
    const matrix& t(*this);
    /**\cond INTERNAL*/
    #define COL(col) v[col]*t(0,col)+v[col]*t(1,col)+v[col]*t(2,col)+v[col]*t(3,col)
    return Vec4(COL(0), COL(1), COL(2), COL(3));
    #undef COL
    /**\endcond*/
  }

  /** Transpose operator */
  matrix operator~() const {
    const matrix& t(*this);
    return matrix(t(0,0), t(1,0), t(2,0), t(3,0),
                  t(0,1), t(1,1), t(2,1), t(3,1),
                  t(0,2), t(1,2), t(2,2), t(3,2),
                  t(0,3), t(1,3), t(2,3), t(3,3));
  }

  /** Invert operator */
  matrix operator!() const;

  operator float*() {
    return v;
  }
};

/** Prints a human-readable representation of a matrix */
std::ostream& operator<<(std::ostream&, const matrix&);

/* Inline so GCC doesn't complain about it not being used. */
/** vector-by-matrix multiplication */
static inline vec4 operator*(const vec4& v, const matrix& t) {
  /**\cond INTERNAL*/
  #define ROW(row) t(row,0)*v[0] + t(row,1)*v[1] + t(row,2)*v[2] + t(row,3)*v[3]
  return Vec4(ROW(0), ROW(1), ROW(2), ROW(3));
  #undef ROW
  /**\endcond*/
}
#endif /* MAT_HXX_ */
