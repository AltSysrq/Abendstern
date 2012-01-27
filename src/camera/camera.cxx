/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/camera/camera.hxx
 */

#include "camera.hxx"
#include "src/globals.hxx"
#include "src/graphics/matops.hxx"

void Camera::setup(bool standard) noth {
  if (standard) {
    mId(matrix_stack::view);
    mTrans(-1, -1, matrix_stack::view);
    mUScale(2, matrix_stack::view);
    mConc(matrix(1,0,0,0,
                 0,1/(vheight=screenH/(float)screenW),0,0,
                 0,0,1,0,
                 0,0,0,1), matrix_stack::view);
    #ifdef AB_OPENGL_14
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, vheight=screenH/(float)screenW, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    #endif

    mId();
  }
  if (reference) doSetup();
}

GameObject* Camera::getReference() const noth {
  return reference;
}

void Camera::setReference(GameObject* ref, bool res) noth {
  reference=ref;
  if (res) reset();
}
