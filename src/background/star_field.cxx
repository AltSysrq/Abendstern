/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/star_field.hxx
 */

#include <cstdlib>
#include <cmath>
#include <iostream>

#include <GL/gl.h>

#include "star_field.hxx"
#include "src/globals.hxx"
#include "src/sim/game_field.hxx"
#include "explosion.hxx"
#include "star.hxx"
#include "src/sim/game_object.hxx"
#include "background_object.hxx"
#include "src/graphics/imgload.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/matops.hxx"
#include "src/exit_conditions.hxx"
using namespace std;

#define SIZE STARFIELD_SIZE
#define FOCUSX cameraCX
#define FOCUSY cameraCY

#define BACKDROP_SIZE 10
#define NUM_BACKDROPS (lenof(backdropVAO))

#define STAR_Z(i) ((i)/(float)starCount*0.15f + 0.05f)
//A 0.1 star takes up 2/1000=0.002 of the screen (radius=0.001) and
//has 5 external vertices. Each bigger one gets an extra vertex
//and +0.001 radius.
#define BASE_RADIUS 0.004f
#define BASE_VERTEX_COUNT 5
#define MAX_RADIUS BASE_RADIUS*10

#define FAKE_ZOOM SF_FAKE_ZOOM

#define DIR "images/star/"
static const char*const textureFiles[] = {
  DIR "bright.png",
  DIR "classic.png",
  DIR "distsun.png",
  DIR "flare.png",
  DIR "flare2.png",
};

#define GLARE_FALL_SEC 0.04f

StarField::StarField(GameObject* ref, GameField* field, int count) {
  glareR=glareG=glareB=0;

  reference=ref;
  this->field=field;
  starCount=count;
  if (ref) {
    oldX=ref->getX();
    oldY=ref->getY();
  } else {
    oldX=oldY=0;
  }
  stars=new Star[count];
  repopulate();

  //Generate backdrops
  if (!headless) {
    unsigned backdropStarCount=screenW*screenH/16.0f/NUM_BACKDROPS*sparkCountMultiplier;

    for (unsigned i=0; i<NUM_BACKDROPS; ++i) {
      backdropSz[i]=backdropStarCount;
      backdropVAO[i]=newVAO();
      glBindVertexArray(backdropVAO[i]);
      backdropVBO[i]=newVBO();
      glBindBuffer(GL_ARRAY_BUFFER, backdropVBO[i]);
      shader::quickV* vertices = new shader::quickV[backdropStarCount];
      for (unsigned n=0; n<backdropStarCount; ++n)
        vertices[n].vertex = Vec2((rand()/(float)RAND_MAX*2.0f - 1.0f)*BACKDROP_SIZE*i,
                                  (rand()/(float)RAND_MAX*2.0f - 1.0f)*BACKDROP_SIZE*i);
      glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices)*backdropStarCount, vertices, GL_STATIC_DRAW);
      shader::quick->setupVBO();
      delete[] vertices;
    }
  }

  //Select background objects
  //We want one from each class; to do this, we select one at random and
  //remove all in that class, and repeat until empty
  //Copy the original (we'll be removing elements)
  vector<BackgroundObject*> bgobjs = backgroundObjects;
  numBkgObjects = backgroundObjectClassCount;
  bkgObjects = new BackgroundObject*[numBkgObjects];
  for (int i=0; i<numBkgObjects; ++i) {
    int index = rand()%bgobjs.size();
    bkgObjects[i] = bgobjs[index];
    bkgObjects[i]->randomize(field->width, field->height);
    bkgObjects[i]->load();

    for (unsigned int j=0; j<bgobjs.size(); ++j)
      if (bgobjs[j]->className==bkgObjects[i]->className)
        bgobjs.erase(bgobjs.begin() + (j--));
  }
  //Sort background objects by size
  //Speed doesn't matter, so just use selection sort
  for (int i=0; i<numBkgObjects; ++i) {
    float min=bkgObjects[i]->getZ();
    BackgroundObject* minObj=bkgObjects[i];
    int index=i;
    for (int j=i+1; j<numBkgObjects; ++j) if (bkgObjects[j]->getZ() < min) {
      min=bkgObjects[j]->getZ();
      index=j;
      minObj=bkgObjects[j];
    }

    //Swap
    if (index!=i) {
      bkgObjects[index]=bkgObjects[i];
      bkgObjects[i]=minObj;
    }
  }

  field->effects=this;
}

StarField::~StarField() {
  delete[] stars;
  for (int i=0; i<numBkgObjects; ++i)
    bkgObjects[i]->unload();
  delete[] bkgObjects;
  if (!headless) {
    for (unsigned i=0; i<4; ++i) {
      glDeleteBuffers(1, &backdropVBO[i]);
      glDeleteVertexArrays(1, &backdropVAO[i]);
    }
  }
  field->effects=&nullEffectsHandler;
}

void StarField::update(float time) noth {
  float glareMul = pow(GLARE_FALL_SEC, time/1000.0f);
  glareR*=glareMul;
  glareG*=glareMul;
  glareB*=glareMul;
  if (!headless) glClearColor(glareR, glareG, glareB, 1.0f);

  if (!reference) return;

  float dx=FOCUSX-oldX,
        dy=FOCUSY-oldY;

  for (int i=0; i<starCount; ++i) {
    stars[i].move(dx, dy);
    if (stars[i].shouldDelete()) {
      bool vert=(rand() & 1);
      float nonExtreme = (rand()/(float)RAND_MAX)*SIZE*2 - SIZE;
      if (vert) {
        //Put the star near the top if the player is heading positive,
        //or near the bottom if negative
        stars[i].reset(nonExtreme, reference->getVY()>0? SIZE : -SIZE+1);
      } else {
        stars[i].reset(reference->getVX()>0? SIZE : -SIZE+1, nonExtreme);
      }
    }
  }

  oldX=FOCUSX;
  oldY=FOCUSY;
}

#define SHEAR_RATE 200.0f
#define SHEAR_THRESH 0.0001f
void StarField::draw() noth {
  BEGINGP("StarField")
  float speed=0, angle=0, angcos, angsin;
  if (reference) {
    float rvx=reference->getVX(),
          rvy=reference->getVY();
    speed=sqrt(rvx*rvx + rvy*rvy);
    if (rvx!=0 && rvy!=0) angle=atan2(rvy, rvx);
    else if (rvy!=0)      angle=(rvy>0? pi/2 : 3*pi/2);
    //We need to translate to compensate for view movement
    //However, simply translate according to the reference
    //So that movement of the camera looks correct as well
    mPush();
    //This DOES work, because the camera has translated the other
    //direction already.
    mTrans(reference->getX(), reference->getY());
  }
  //Realistic scaling (sortof...)
  GLfloat fakeZoom = FAKE_ZOOM;
  GLfloat fakeZoomMul = cameraZoom/fakeZoom;
  mScale(1/fakeZoomMul, 1/fakeZoomMul);

  for (unsigned i=0; i<NUM_BACKDROPS; ++i) {
    if (reference) {
      GameField* field=reference->getField();
      float size=(field->width>field->height? field->width : field->height);
      //Parallax will only move the backdrop i screens across the entire
      //field
      mPush();
      mTrans( -(reference->getX() - field->width/2)/size*(i+1),
              -(reference->getY() - field->height/2)/size*(i+1));
    }
    glBindVertexArray(backdropVAO[i]);
    glBindBuffer(GL_ARRAY_BUFFER, backdropVBO[i]);
    const shader::quickU uni = {{{0.5f,0.5f,0.5f,1.0f}}};
    shader::quick->activate(&uni);
    glDrawArrays(GL_POINTS, 0, backdropSz[i]);
    if (reference) mPop();
  }

  float xd=(cameraX2-cameraX1)/2+MAX_RADIUS, yd=(cameraY2-cameraY1)/2+MAX_RADIUS;
  float minX=(cameraX1-xd-FOCUSX)*fakeZoomMul,
        minY=(cameraY1-yd-FOCUSY)*fakeZoomMul,
        maxX=(cameraX2+xd-FOCUSX)*fakeZoomMul,
        maxY=(cameraY2+yd-FOCUSY)*fakeZoomMul;
  angcos=cos(angle);
  angsin=sin(angle);
  BackgroundObject** bkgObjects = this->bkgObjects;
  float rx = cameraCX, ry=cameraCY;
  int numBkgObjects = field->width > 25? this->numBkgObjects : 0;

  //Don't bother drawing stars that are less than 1 px
  float minSize=0.5f/(float)screenW/fakeZoomMul;
  float speedShear=speed*SHEAR_RATE/fakeZoomMul;
  for (int i=0; i<starCount; ++i) {
    float z = STAR_Z(i);
    while (numBkgObjects && z > (*bkgObjects)->getZ()) {
      (*bkgObjects)->draw(rx, ry);
      ++bkgObjects;
      --numBkgObjects;
    }

    float size=BASE_RADIUS*(int)(stars[i].getZ()*10);
    if (size<minSize) continue;

    if (stars[i].getX()>minX && stars[i].getX()<maxX &&
        stars[i].getY()>minY && stars[i].getY()<maxY)
      stars[i].draw(speedShear, angle, angcos, angsin);
  }

  while (numBkgObjects) {
    (*bkgObjects)->draw(rx, ry);
    ++bkgObjects;
    --numBkgObjects;
  }

  if (reference) mPop();
  ENDGP
}

void StarField::updateReference(GameObject* ref, bool reset) noth {
  if (reset) glareR=glareG=glareB=0;
  if (!reference || reset) {
    if (!ref) return;

    repopulate();
    reference=ref;
    if (reference) {
      oldX=reference->getX();
      oldY=reference->getY();
    }
  } else {
/*    oldX=FOCUSX;
    oldY=FOCUSY;
    //Handle the sudden movement correctly
    update(0);
*/
    reference=ref;
  }
}

void StarField::explode(Explosion* ex) noth {
  if (!reference) return;
  float dx=reference->getX()-ex->getX(), dy=reference->getY()-ex->getY();
  float dist=sqrt(dx*dx + dy*dy);
  if (dist == 0) dist=0.00000001f;
  float mul=ex->effectsDensity*ex->getSize()/dist;
  float r = ex->getColourR()*mul,
        g = ex->getColourG()*mul,
        b = ex->getColourB()*mul;
  glareR+=r;
  glareG+=g;
  glareB+=b;
}

void StarField::repopulate() noth {
  //Instead of using random distances, we
  //give each star a distance according to its
  //position in the array. This means that
  //The stars will be drawn in order of distance
  for (int i=0; i<starCount; ++i)
    stars[i].reset(rand()/(float)RAND_MAX*SIZE*2 - SIZE,
                   rand()/(float)RAND_MAX*SIZE*2 - SIZE,
                   STAR_Z(i));
  //Reset coords
  if (reference) {
    oldX=reference->getX();
    oldY=reference->getY();
  }
}

void initStarLists() {
  if (headless) return;
  glGenTextures(lenof(textureFiles), starTextures);
  for (unsigned i=lenof(textureFiles); i<lenof(starTextures); ++i)
    starTextures[i]=0;
  for (unsigned i=0; i<lenof(textureFiles); ++i) {
    if (const char* error = loadImageGrey(textureFiles[i], starTextures[i])) {
      cerr << "Unable to load star texture " << textureFiles[i] << ": " << error << endl;
      exit(EXIT_MALFORMED_DATA);
    }
  }
}
