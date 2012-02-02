/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/matops.hxx
 */

/*
 * matops.cxx
 *
 *  Created on: 20.01.2011
 *      Author: jason
 */

#include <stack>
#include <cmath>
#include <cassert>
#include <iostream>

#include "matops.hxx"
#include "src/fasttrig.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"

using namespace std;

static matrix identity;
static stack<matrix> viewStack, modelStack;
void*const matrix_stack::view = &viewStack;
void*const matrix_stack::model = &modelStack;

#define ENTER_PROJECTION(stptr) if (stptr == matrix_stack::view) glMatrixMode(GL_PROJECTION)
#define EXIT_PROJECTION(stptr)  if (stptr == matrix_stack::view) glMatrixMode(GL_MODELVIEW)

void mInit() noth {
  viewStack.push(::identity);
  modelStack.push(::identity);
}

#define STACK stack<matrix>& st(*(stack<matrix>*)stptr)

void mPush(void* stptr) noth {
  STACK;
  matrix tmp(st.top());
  st.push(tmp);
  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glPushMatrix();
  EXIT_PROJECTION(stptr);
  #endif
}

void mPop(void* stptr) noth {
  STACK;
  st.pop();
  assert(!st.empty());

  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glPopMatrix();
  EXIT_PROJECTION(stptr);
  #endif
}

float* mGet(void* stptr) noth {
  STACK;
  return (float*)st.top();
}

void mId(void* stptr, bool bypass) noth {
  STACK;
  st.top() = (bypass || stptr != matrix_stack::model? ::identity : viewStack.top());
  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glLoadIdentity();
  EXIT_PROJECTION(stptr);
  #endif
}

void mLoad(const matrix& mat, void* stptr, bool bypass) noth {
  STACK;
  st.top() = (bypass || stptr != matrix_stack::model? mat : mat * viewStack.top());
}

void mConc(const matrix& mat, void* stptr) noth {
  STACK;
  st.top() = mat*st.top();
}

void mTrans(float x, float y, void* stptr) noth {
  mConc(matrix(1, 0, 0, x,
               0, 1, 0, y,
               0, 0, 1, 0,
               0, 0, 0, 1), stptr);
  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glTranslatef(x, y, 0);
  EXIT_PROJECTION(stptr);
  #endif
}

void mRot(float theta, void* stptr) noth {
  float s = fsin(theta), c=fcos(theta);
  mConc(matrix(c, -s, 0, 0,
               s, c, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1), stptr);
  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glRotatef(theta/pi*180.0f, 0, 0, 1);
  EXIT_PROJECTION(stptr);
  #endif
}

void mUScale(float fac, void* stptr) noth {
  mConc(matrix(fac, 0, 0, 0,
               0, fac, 0, 0,
               0, 0, fac, 0,
               0, 0, 0, 1), stptr);
  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glScalef(fac,fac,fac);
  EXIT_PROJECTION(stptr);
  #endif
}

void mScale(float x, float y, void* stptr) noth {
  mConc(matrix(x, 0, 0, 0,
               0, y, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1), stptr);
  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glScalef(x,y,1);
  EXIT_PROJECTION(stptr);
  #endif
}

void mClear(void* stptr) noth {
  STACK;
  //st.clear();
  while (!st.empty() && 1 != st.size()) mPop(stptr);
  if (st.empty()) st.push(::identity);
  else            st.top() = ::identity;

  #ifdef AB_OPENGL_14
  ENTER_PROJECTION(stptr);
  glLoadIdentity();
  EXIT_PROJECTION(stptr);
  #endif
}

unsigned mSize(void* stptr) noth {
  STACK;
  return st.size();
}
