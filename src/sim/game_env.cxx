/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/game_env.hxx
 */

#include <GL/gl.h>

#include "game_env.hxx"
#include "src/globals.hxx"
#include "src/secondary/global_chat.hxx"
#include "src/background/background.hxx"
#include "src/camera/dynamic_camera.hxx"

GameEnv::GameEnv(Camera* c, float w, float h) :
  field(w,h),
  stars(NULL),
  cam(c) {}

GameEnv::GameEnv(float w, float h):
  field(w,h),
  stars(NULL),
  cam(new DynamicCamera(NULL, &field))
{
}

GameEnv::~GameEnv() {
  if (stars) delete stars;
  delete cam;
}

GameObject* GameEnv::getReference() noth {
  return cam->getReference();
}

void GameEnv::setReference(GameObject* ref, bool reset) {
  cam->setReference(ref, reset);
  if (stars) stars->updateReference(ref, reset);
  if (reset && ref) {
    cameraCX=ref->getX();
    cameraCY=ref->getY();
  }
}

void GameEnv::update(float t) {
  //We need to update the background first so that the Nebula
  //can properly coordinate with its background thread
  if (stars) stars->update(t);
  field.update(t);
  cam->update(t);
  global_chat::update(t);
}

void GameEnv::draw() {
  cam->setup();
  if (stars) stars->draw();
  field.draw();
  if (stars) stars->postDraw();
  cam->drawOverlays();
}
