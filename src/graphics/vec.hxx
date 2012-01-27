/**
 * @file
 * @author Jason Lingle
 * @brief Defines operations on 2-, 3-, and 4-component vectors.
 *
 * All functions are inline to maximise performance.
 * Operators:
 * \verbatim
 *   +, -       Normal
 *   *          Componentwise multiplication
 *   |          Dot product
 *   &          Cross product
 *   ~          Return length
 *   !          Normalize (destructive)
 * \endverbatim
 * Note that the vec4 treats the fourth coordinate as homogeneous w,
 * for purposes of dot and cross products, length, and for implicit
 * conversion to vectors of lower order.
 *
 * In order to classify as PODs under C++03, these classes have
 * no true constructors. Instead, use the global functions starting
 * with capital letters.
 */
/*
 * vec.hxx
 *
 *  Created on: 20.01.2011
 *      Author: jason
 */

#ifndef VEC_HXX_
#define VEC_HXX_

#include <cmath>

struct vec2;
/** Pseudo-constructor for vec2 */
static inline vec2 Vec2(const vec2&);
/** Pseudo-constructor for vec2 */
static inline vec2 Vec2(float=0,float=0);
/** Pseudo-constructor for vec2 */
static inline vec2 Vec2(const float*);
struct vec3;
/** Pseudo-constructor for vec3 */
static inline vec3 Vec3(const vec2&, float=0);
/** Pseudo-constructor for vec3 */
static inline vec3 Vec3(float=0,float=0,float=0);
/** Pseudo-constructor for vec3 */
static inline vec3 Vec3(const vec3&);
/** Pseudo-constructor for vec3 */
static inline vec3 Vec3(const float*);
struct vec4;
/** Pseudo-constructor for vec4 */
static inline vec4 Vec4(const vec2&, float=0,float=1);
/** Pseudo-constructor for vec4 */
static inline vec4 Vec4(const vec3&, float=1);
/** Pseudo-constructor for vec4 */
static inline vec4 Vec4(float=0,float=0,float=0,float=1);
/** Pseudo-constructor for vec4 */
static inline vec4 Vec4(const vec4&);
/** Pseudo-constructor for vec4 */
static inline vec4 Vec4(const float*);

/** A two-dimensional vector */
struct vec2 {
  float v[2];

  inline float operator[](unsigned i) const {
    return v[i];
  }
  inline float& operator[](unsigned i) {
    return v[i];
  }

  /*vec2& operator=(const vec2& o) {
    v[0]=o[0];
    v[1]=o[1];
    return *this;
  }*/

  vec2& operator=(const float* src) {
    v[0]=src[0];
    v[1]=src[1];
    return *this;
  }

  vec2 operator+(const vec2& o) const {
    return Vec2(v[0]+o[0], v[1]+o[1]);
  }
  vec2 operator-(const vec2& o) const {
    return Vec2(v[0]-o[0], v[1]+o[1]);
  }
  vec2& operator+=(const vec2& o) {
    v[0]+=o[0];
    v[1]+=o[1];
    return *this;
  }
  vec2& operator-=(const vec2& o) {
    v[0]-=o[0];
    v[1]-=o[1];
    return *this;
  }

  vec2 operator*(const vec2& o) const {
    return Vec2(v[0]*o[0], v[1]*o[1]);
  }
  vec2& operator*=(const vec2& o) {
    v[0]*=o[0];
    v[1]*=o[1];
    return *this;
  }
  vec2 operator*(float f) const {
    return Vec2(v[0]*f, v[1]*f);
  }
  vec2& operator*=(float f) {
    v[0]*=f;
    v[1]*=f;
    return *this;
  }

  /** Dot product */
  float operator|(const vec2& o) const {
    return v[0]*o[0] + v[1]*o[1];
  }

  /** Cross product.
   * The cross product is a scalar in two dimensions.
   */
  float operator&(const vec2& o) const {
    return v[0]*o.v[1] - v[1]*o.v[0];
  }

  /** Magnitude */
  float operator~() const {
    return std::sqrt(v[0]*v[0] + v[1]*v[1]);
  }

  /** Destructively normalises this vector.
   * @return *this
   */
  vec2& operator!() {
    float l=~*this;
    v[0]/=l;
    v[1]/=l;
    return*this;
  }
};

/** A three-dimensional vector */
struct vec3 {
  float v[3];

  inline float operator[](unsigned i) const {
    return v[i];
  }
  inline float& operator[](unsigned i) {
    return v[i];
  }

  vec3& operator=(const vec2& o) {
    v[0]=o[0];
    v[1]=o[1];
    v[2]=0;
    return *this;
  }

/*  vec3& operator=(const vec3& o) {
    v[0]=o[0];
    v[1]=o[1];
    v[2]=o[2];
    return *this;
  }*/

  vec3& operator=(const float* o) {
    v[0]=o[0];
    v[1]=o[1];
    v[2]=o[2];
    return *this;
  }

  vec3 operator+(const vec3& o) const {
    return Vec3(v[0]+o[0], v[1]+o[1], v[2]+o[2]);
  }
  vec3& operator+=(const vec3& o) {
    v[0]+=o[0];
    v[1]+=o[1];
    v[2]+=o[2];
    return *this;
  }
  vec3 operator-(const vec3& o) const {
    return Vec3(v[0]-o[0], v[1]-o[1], v[2]-o[2]);
  }
  vec3& operator-=(const vec3& o) {
    v[0]-=o[0];
    v[1]-=o[1];
    v[2]-=o[2];
    return *this;
  }
  vec3 operator*(const vec3& o) const {
    return Vec3(v[0]*o[0], v[1]*o[1], v[2]*o[2]);
  }
  vec3& operator*=(const vec3& o) {
    v[0]*=o[0];
    v[1]*=o[1];
    v[2]*=o[2];
    return *this;
  }
  vec3 operator*(float f) const {
    return Vec3(v[0]*f,v[1]*f,v[2]*f);
  }
  vec3& operator*=(float f) {
    v[0]*=f;
    v[1]*=f;
    v[2]*=f;
    return *this;
  }

  /** Dot product */
  float operator|(const vec3& o) const {
    return v[0]*o[0] + v[1]*o[1] + v[2]*o[2];
  }

  /** Cross product */
  vec3 operator&(const vec3& o) const {
    return Vec3(v[1]*o[2] - v[2]*o[1],
                v[2]*o[0] - v[0]*o[2],
                v[0]*o[1] - v[1]*o[0]);
  }
  /** Assignment cross product */
  vec3& operator&=(const vec3& o) {
    return (*this) = (*this)&o;
  }

  /** Magnitude */
  float operator~() const {
    return std::sqrt(v[0]*v[0] + v[1]*v[1]+v[2]*v[2]);
  }
  /** Destructively normalises this vector.
   * @return *this
   */
  vec3& operator!() {
    float l=~*this;
    v[0]/=l;
    v[1]/=l;
    v[2]/=l;
    return *this;
  }
};

/** A three-dimensional vector plus w component */
struct vec4 {
  float v[4];

  inline float operator[](unsigned i) const {
    return v[i];
  }
  inline float& operator[](unsigned i) {
    return v[i];
  }

  vec4& operator=(const vec2& o) {
    v[0]=o[0];
    v[1]=o[0];
    v[2]=0;
    v[3]=1;
    return *this;
  }
  vec4& operator=(const vec3& o) {
    v[0]=o[0];
    v[1]=o[1];
    v[2]=o[2];
    v[3]=1;
    return *this;
  }
  /*vec4& operator=(const vec4& o) {
    v[0]=o[0];
    v[1]=o[1];
    v[2]=o[2];
    v[3]=o[3];
    return *this;
  }*/
  vec4& operator=(const float* f) {
    v[0]=f[0];
    v[1]=f[1];
    v[2]=f[2];
    v[3]=f[3];
    return *this;
  }

  vec4 operator+(const vec4& o) const {
    return Vec4(v[0]+o[0], v[1]+o[1], v[2]+o[2], v[3]+o[3]);
  }
  vec4& operator+=(const vec4& o) {
    v[0]+=o[0];
    v[1]+=o[1];
    v[2]+=o[2];
    v[3]+=o[3];
    return *this;
  }
  vec4 operator-(const vec4& o) const {
    return Vec4(v[0]-o[0], v[1]-o[1], v[2]-o[2], v[3]-o[3]);
  }
  vec4& operator-=(const vec4& o) {
    v[0]-=o[0];
    v[1]-=o[1];
    v[2]-=o[2];
    v[3]-=o[3];
    return *this;
  }
  vec4 operator*(const vec4& o) const {
    return Vec4(v[0]*o[0], v[1]*o[1], v[2]*o[2], v[3]*o[3]);
  }
  vec4& operator*=(const vec4& o) {
    v[0]*=o[0];
    v[1]*=o[1];
    v[2]*=o[2];
    v[3]*=o[3];
    return *this;
  }
  vec4 operator*(float f) const {
    return Vec4(v[0]*f, v[1]*f, v[2]*f, v[3]*f);
  }
  vec4& operator*=(float f) {
    v[0]*=f;
    v[1]*=f;
    v[2]*=f;
    v[3]*=f;
    return *this;
  }

  /* For the dot and cross products, assume we are in
   * homogeneous coordinates and not 4D.
   */
  /** Dot product. */
  float operator|(const vec4& o) const {
    return (v[0]*o[0] + v[1]*o[1] + v[2]*o[2])
         / (v[3]*o[3]); //Divide through by w
  }

  /** Cross product */
  vec4 operator&(const vec4& o) const {
    return Vec4(v[1]*o[2]-v[2]*o[1],
                v[2]*o[0]-v[0]*o[2],
                v[0]*o[1]-v[1]*o[0],
                v[3]*o[3]);
  }
  /** Assigned cross product */
  vec4& operator&=(const vec4& o) {
    return (*this) = (*this) & o;
  }

  /* Length and normalization shall also take w into account. */
  /** Magnitude */
  float operator~() const {
    return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])/v[3];
  }
  /** Destructively normalises this vector.
   *
   * @return *this
   */
  vec4& operator!() {
    float l=~*this;
    v[0]/=l;
    v[1]/=l;
    v[2]/=l;
    v[3]=1;
    return *this;
  }
};

static inline vec2 Vec2(const vec2& v) {
  return v;
}

static inline vec2 Vec2(float x, float y) {
  vec2 v;
  v[0]=x;
  v[1]=y;
  return v;
}

static inline vec2 Vec2(const float* f) {
  vec2 v;
  v[0]=f[0];
  v[1]=f[1];
  return v;
}

static inline vec3 Vec3(const vec2& xy, float z) {
  vec3 v;
  v[0]=xy[0];
  v[1]=xy[1];
  v[2]=z;
  return v;
}

static inline vec3 Vec3(float x, float y, float z) {
  vec3 v;
  v[0]=x;
  v[1]=y;
  v[2]=z;
  return v;
}

static inline vec3 Vec3(const vec3& v) {
  return v;
}

static inline vec3 Vec3(const float* f) {
  vec3 v;
  v[0]=f[0];
  v[1]=f[1];
  v[2]=f[2];
  return v;
}

static inline vec4 Vec4(const vec2& xy, float z, float w) {
  vec4 v;
  v[0]=xy[0];
  v[1]=xy[1];
  v[2]=z;
  v[3]=w;
  return v;
}

static inline vec4 Vec4(const vec3& xyz, float w) {
  vec4 v;
  v[0]=xyz[0];
  v[1]=xyz[1];
  v[2]=xyz[2];
  v[3]=w;
  return v;
}

static inline vec4 Vec4(const vec4& v) {
  return v;
}

static inline vec4 Vec4(float x, float y, float z, float w) {
  vec4 v;
  v[0]=x;
  v[1]=y;
  v[2]=z;
  v[3]=w;
  return v;
}

static inline vec4 Vec4(const float* f) {
  vec4 v;
  v[0]=f[0];
  v[1]=f[1];
  v[2]=f[2];
  v[3]=f[3];
  return v;
}

#endif /* VEC_HXX_ */
