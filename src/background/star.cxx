/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/star.hxx
 */

#include <cstdlib>
#include <iostream>
#include <cmath>

#include <GL/gl.h>

#include "star.hxx"
#include "src/globals.hxx"
#include "star_field.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/exit_conditions.hxx"
using namespace std;

GLuint starTextures[9];

static GLuint vao, vbo;
static const shader::textureModulateV vertices[4] = {
    { {{-0.5,-0.5,0,1}}, {{0,0}} },
    { {{-0.5,+0.5,0,1}}, {{0,1}} },
    { {{+0.5,-0.5,0,1}}, {{1,0}} },
    { {{+0.5,+0.5,0,1}}, {{1,1}} },
};

void Star::reset(float x, float y, float dist) noth {
  if (headless) return;
  this->x=x;
  this->y=y;
  distance=dist;
  do {
    tex = rand() % lenof(starTextures);
  } while (starTextures[tex] == 0);

  //A star can be reddish (subtract from green and blue),
  //yellowish (subtract from blue), or bluish (subtract from red)
  //We usually get very low numbers for subtraction, though,
  //so most stars are nearly white
  //Colour chances: red 25%, yellow 50%, blue 25%
  //We set reddish even if yellowish because it simplifies things below
  bool yellowish=rand()&1,
       reddish = yellowish || (rand()&1),
       blueish = !yellowish;
  float mainSubtract=rand()/(float)RAND_MAX;
  //Steepen the curve: 0.5->0.25->0.125->0.0625
  mainSubtract=mainSubtract* //0.5
               mainSubtract* //0.25
               mainSubtract* //0.125
               mainSubtract; //0.0625
  //From green when yellowish. We don't want magenta stars,
  //so multiply by mainSubtract instead of tesseracting it.
  //Also from green when blueish, so stars tend to be more
  //cyan than blue
  float secondSubtract=mainSubtract * (rand()/(float)RAND_MAX);

  if (yellowish) {
    colourR=1;
    colourG=1-secondSubtract;
    colourB=1-mainSubtract;
  } else if (reddish) {
    colourR=1;
    colourG=1-mainSubtract;
    colourB=1-mainSubtract;
  } else if (blueish) {
    colourR=1-mainSubtract;
    colourG=1-secondSubtract;
    colourB=1;
  } else {
    std::cerr << "Something is seriously wrong with the star colour "
    "selection algorithm!" << std::endl;
    std::exit(EXIT_PROGRAM_BUG);
  }
}

void Star::move(float dx, float dy) noth {
  x -= dx*distance*SF_FAKE_ZOOM;
  y -= dy*distance*SF_FAKE_ZOOM;
}

#define BASE_RADIUS 0.04f
void Star::draw(float shear, float angle, float cosAngle, float sinAngle) noth {
  BEGINGP("Star")

  static bool hasLoadedVAO=false;
  if (!hasLoadedVAO) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::textureModulate->setupVBO();
    hasLoadedVAO=true;
  }

  mPush();
  float size = BASE_RADIUS*distance;
  mTrans(x, y);
  mRot(angle);
  mScale((1+shear)*size, size);
  glBindTexture(GL_TEXTURE_2D, starTextures[tex]);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  shader::textureModulateU uni;
  uni.modColour = Vec4(colourR, colourG, colourB, 1);
  uni.colourMap = 0;
  shader::textureModulate->activate(&uni);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  mPop();
  ENDGP
}

bool Star::shouldDelete() noth {
  return x<-STARFIELD_SIZE+1 ||
         x>+STARFIELD_SIZE   ||
         y<-STARFIELD_SIZE+1 ||
         y>+STARFIELD_SIZE;
}
