/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/background_object.hxx
 */

#include <vector>
#include <stdexcept>
#include <exception>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include <GL/gl.h>
#include <libconfig.h++>

#include <SDL.h>
#include <SDL_image.h>

#include "background_object.hxx"
#include "src/globals.hxx"
#include "src/graphics/imgload.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"

using namespace std;
using namespace libconfig;

#define IMAGE_FORMAT "images/bg/%s.png"
#define IMAGE_RC "images/bg/bg.rc"

vector<BackgroundObject*> backgroundObjects;
unsigned backgroundObjectClassCount=0;
static GLuint vao, vbo;

static const shader::textureReplaceV vertices[4] = {
    { {{-0.5f,-0.5f,0,1}}, {{0,1}} },
    { {{+0.5f,-0.5f,0,1}}, {{1,1}} },
    { {{-0.5f,+0.5f,0,1}}, {{0,0}} },
    { {{+0.5f,+0.5f,0,1}}, {{1,0}} },
};

BackgroundObject::~BackgroundObject() {
  unload();
}

void BackgroundObject::randomize(float maxx, float maxy) noth {
  float minz=1/minDist, maxz=1/maxDist;
  z = 1 / (rand()/(float)RAND_MAX * (maxz-minz) + minz);

  x = rand()/(float)RAND_MAX * maxx;
  y = rand()/(float)RAND_MAX * maxy;
}

float BackgroundObject::getX() const noth { return x; }
float BackgroundObject::getY() const noth { return y; }
float BackgroundObject::getZ() const noth { return z; }

void BackgroundObject::draw(float rx, float ry) const noth {
  BEGINGP("BackgroundObject")
  mPush();
  mTrans((x-rx)*z, (y-ry)*z);
  float size=vheight * natSize * z/natDist;
  mUScale(size);
  glBindTexture(GL_TEXTURE_2D, texture);
  shader::textureReplaceU uni;
  uni.colourMap=0;
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  shader::textureReplace->activate(&uni);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  mPop();
  glBindTexture(GL_TEXTURE_2D, 0);
  ENDGP
}

const char* BackgroundObject::load() noth {
  if (headless) return NULL;
  static bool hasVAO=false;
  if (!hasVAO) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::textureReplace->setupVBO();
    hasVAO=true;
  }

  glGenTextures(1, &texture);
  const char* err=loadImage(filename.c_str(), texture);
  return err;
}

void BackgroundObject::unload() noth {
  if (texture) glDeleteTextures(1, &texture);
}

bool loadBackgroundObjects() {
  try {
    Config rc;
    rc.readFile(IMAGE_RC);
    Setting& classList=rc.getRoot()["class_list"];
    backgroundObjectClassCount=classList.getLength();
    for (int i=0; i<classList.getLength(); ++i) {
      Setting& conf=rc.getRoot()[(const char*)classList[i]];
      for (int j=0; j<conf["list"].getLength(); ++j) {
        char filename[256];
        const char* name=conf["list"][j];
        sprintf(filename, IMAGE_FORMAT, name);

        backgroundObjects.push_back(new BackgroundObject(filename,
                                                         conf["min_distance"],
                                                         conf["natural_distance"],
                                                         conf["max_distance"],
                                                         conf["natural_size"],
                                                         classList[i]));
      }
    }
  } catch (exception& e) {
    cout << "Error reading background: " << e.what() << endl;
    return false;
  }

  return true;
}
