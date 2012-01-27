/**
 * @file
 * @author Jason Lingle
 * @brief Important matrix operations, a-la the old OpenGL API.
 *
 *
 * It provides a matrix stack as well as an "effective
 * identity" matrix stack.
 */

/*
 * matops.hxx
 *
 *  Created on: 20.01.2011
 *      Author: jason
 */

#ifndef MATOPS_HXX_
#define MATOPS_HXX_

#include "mat.hxx"
#include "src/opto_flags.hxx"

/** Contains the two matrix stacks. */
namespace matrix_stack {
  /** The secondary view matrix stack.
   * The actual type of this object is undefined.
   */
  extern void*const view;
  /** The primary matrix stack.
   * The actual type of this object is undefined.
   */
  extern void*const model;
}

/** Initializes the matrix stacks. */
void mInit() noth;

/** Duplicates the top of the given matrix stack,
 * defaulting to the model stack.
 */
void mPush(void* s = matrix_stack::model) noth;

/** Drops the top of the given matrix stack,
 * defaulting to the model stack.
 */
void mPop(void* s = matrix_stack::model) noth;

/** Returns the current top of the given matrix stack,
 * defaulting to the model stack.
 * The return value should not be modified, although
 * it is non-const since C functions can't take
 * const objects.
 */
float* mGet(void* s = matrix_stack::model) noth;

/** Loads the effective identity into the top of the
 * given stack, defaulting to the model stack. If
 * the model stack is passed, the current view matrix
 * is used instead of the identity itself, unless
 * bypassMul is true.
 */
void mId(void* s = matrix_stack::model, bool bypassMul=false) noth;

/** Loads the specified matrix into the top of the
 * given stack, defaulting to the model stack.
 * If the model stack is passed, the matrix is
 * first multiplied with the current view matrix,
 * unless bypass is true.
 * In OpenGL 2.1 compatibility mode, this call will
 * NOT alter OpenGL's native matrix stack. You must
 * also make an equivilant call to OpenGL.
 */
void mLoad(const matrix&, void* s = matrix_stack::model, bool bypassMul=false) noth;

/** Concatenates the specified matrix with the top of
 * the given stack, defaulting to the model stack.
 * In OpenGL 2.1 compatibility mode, this call will
 * NOT alter OpenGL's native matrix stack. You must
 * also make an equivilant call to OpenGL.
 */
void mConc(const matrix&, void* s = matrix_stack::model) noth;

/** Concatenates a translation onto the specified stack.
 * Since this is Abendstern, this is a 2D-only translation.
 */
void mTrans(float x, float y, void* s = matrix_stack::model) noth HOT;

/** Concatenates a rotation onto the specified stack.
 * Angle is in radians. Always around Z axis.
 */
void mRot(float theta, void* s = matrix_stack::model) noth HOT;

/** Concatenates a uniform scale onto the specified stack. */
void mUScale(float fac, void* s = matrix_stack::model) noth HOT;

/** Concatenates a possibly nonuniform scale onto the specified stack. */
void mScale(float x, float y, void* s = matrix_stack::model) noth HOT;

/** Clear the given stack, then add a single true identity. */
void mClear(void* s = matrix_stack::model) noth;

/** Returns the size of the given stack. */
unsigned mSize(void* s = matrix_stack::model) noth;

#endif /* MATOPS_HXX_ */
