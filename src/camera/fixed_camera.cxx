/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/camera/fixed_camera.hxx
 */

#include <GL/gl.h>
#include "fixed_camera.hxx"
#include "src/globals.hxx"

void FixedCamera::doSetup() noth {
  glTranslatef(-reference->getX()+0.5f, -reference->getY()+vheight/2, 0);
  cameraX1=reference->getX()-0.5f;
  cameraX2=reference->getX()+0.5f;
  cameraY1=reference->getY()-vheight/2;
  cameraY2=reference->getY()+vheight/2;
}
