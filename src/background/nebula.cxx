/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/nebula.hxx
 */
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <utility>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#if !defined(DEBUG) && !defined(NDEBUG)
#define NDEBUG
#endif
#include <cassert>

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_thread.h>

#include "nebula.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/square.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "explosion.hxx"
#include "src/globals.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

/* Implementation details:
 * Threads:
 * In order to maximise efficiency, almost all nebula computation is run on a
 * separate thread. Exactly one thread at a time is allowed to access the objects
 * at a time (drawing in this case will not count as access); likewise the GPU.
 * When Nebula::update is called, it releases gpuLock if it is holding it, then
 * locks updateLock, then marks that it holds that. Similarly, Nebula::draw
 * releases updateLock if held, then locks and records gpuLock.
 * In headless mode, since draw will never be called, update will release updateLock,
 * sleep 4 milliseconds, then take updateLock again.
 * While the slave has the updateLock, it copies the NebulaResistanceElements from
 * each object, calculates interactions, then writes changes back into the object.
 * When it has the gpuLock, it sends the copied resistance information to the GPU
 * to update the pressure/velocity maps thereon.
 * The slave thread exits when alive is set to false.
 *
 * Updating on the GPU is performed as follows. First, the nebula_base_update shader is called
 * to draw the back texture onto the front, with the elapsed time passed as a uniform
 * (as well as the other nebula parms). This performs the base update (described below) in
 * the frag shader, copying and updating data from the back texture into the front. Then, the
 * nebula_object_update shader is called with points for each NebulaResistanceElement, the
 * positions and velocities being passed in. This performs the nebula object updating described
 * below for each point (a geometry shader expands it into the four points that will have
 * interactions). Only NebulaResistanceElements that are known to be relevent are sent.
 *
 * Base update:
 * The new pressure for a coordinate x,y is given by (where v are the velocities from the previous
 * frame and p are the pressures):
 *   p(x,y) + time*(max(0, p(x+1,y)*v(x+1,y) dot <-1,0>) + max(0, p(x-1,y)*v(x-1,y) dot <1,0>)
 *                 +max(0, p(x,y+1)*v(x,y+1) dot <0,-1>) + max(0, p(x,y-1)*v(x,y-1) dot <0,1>)
 *                 - p(x,y)*v(x,y))
 *          + time*prubber*(natural_pressure(x,y)-p(x,y))
 * The new velocity for a coordinate x,y is calculated with:
 *   v(x,y) + time/mass/p(x,y)*((p(x,y)-p(x+1,y))*<1,0> + (p(x,y)-p(x-1,y))*<-1,0>
 *                             +(p(x,y)-p(x,y+1))*<0,1> + (p(x,y)-p(x,y-1))*<0,-1>
 *                             +viscosity*((v(x+1,y)*p(x+1,y)-v(x,y)*p(x,y))
 *                                        +(v(x-1,y)*p(x-1,y)-v(x,y)*p(x,y))
 *                                        +(v(x,y+1)*p(x,y+1)-v(x,y)*p(x,y))
 *                                        +(v(x,y-1)*p(x,y-1)-v(x,y)*p(x,y))))
 *          + time*vrubber*(natural_velocity(x,y)-v(x,y))
 * The edges of the grid are fixed to their natural values.
 *
 * Nebula object update:
 * A NebulaResistanceElement interacts with the four points surrounding it. Given an element at
 * N at velocity V and a point at P, the velocity is added with the below value, if (N-P) dot (V-v(P))
 * is greater than 0:
 *   1000 * V * normalise((N-P) dot V)
 * The force exerted on that point is simply forcemul*p(P)*density*(v(P)-V).
 */

#define GLARE_FALL_SEC 0.04f
#define UPDATE_STEP 5.0f
#define MAX_FEEDBACK_OBJECTS 4096

#if defined(AB_OPENGL_14) || defined(AB_OPENGL_21)
  #define ENABLE_THREADING (!headless)
#else
  #define ENABLE_THREADING false
#endif

/* Attempts to parse the given sequence of strings as an equation.
 * Throws const char* on error.
 * The first argument will be modified to reflect the new location.
 */
template<typename It>
static nebula_equation::Element* interpSub(It& curr, It end) throw (const char*) {
  if (curr >= end) throw "Unexpected end of equation!";
  const string& s(*curr++);
  nebula_equation::Element* a=NULL, *b=NULL, *c=NULL;
  try {
    #define ZER(str,cls) else if (s == str) { return new nebula_equation::cls; }
    #define UNA(str,cls) else if (s == str) { a=interpSub(curr, end); return new nebula_equation::cls(a); }
    #define BIN(str,cls) else if (s == str) { a=interpSub(curr, end); b=interpSub(curr,end); return new nebula_equation::cls(a,b); }
    #define TRI(str,cls) else if (s == str) { a=interpSub(curr, end); b=interpSub(curr,end); c=interpSub(curr,end); return new nebula_equation::cls(a,b,c); }
    if (s.size() > 0 && isdigit(s[0])) return new nebula_equation::Constant(atof(s.c_str()));
    ZER("pi",Constant(pi))
    ZER("x", X)
    ZER("y", Y)
    BIN("+", Add)
    BIN("-", Sub)
    BIN("*", Mul)
    BIN("/", Div)
    BIN("^", Pow)
    BIN("**", Pow)
    BIN("%", Mod)
    UNA("_", Neg)
    UNA("cos", Cos)
    UNA("sin", Sin)
    UNA("tan", Tan)
    UNA("cosh", Cosh)
    UNA("sinh", Sinh)
    UNA("tanh", Tanh)
    UNA("acos", Acos)
    UNA("asin", Asin)
    UNA("atan", Atan)
    BIN("atan2", Atan2)
    UNA("sqrt", Sqrt)
    UNA("abs", Abs)
    UNA("||",  Abs)
    UNA("|",   Abs)
    UNA("e^",  Exp)
    UNA("e**", Exp)
    UNA("ln",  Ln)
    UNA("log", Log)
    UNA("ceil",Ceil)
    UNA("|^",  Ceil)
    UNA("floor",Floor)
    UNA("|_",  Floor)
    BIN("min", Min)
    BIN("max", Max)
    else throw "Unknown function";
    #undef UNA
    #undef BIN
  } catch (...) {
    if (a) delete a;
    if (b) delete b;
    if (c) delete c;
    throw;
  }
}

/* Common function for equation parsing.
 * If successful, sets the given Element* to the parsed value,
 * deleting the old value if non-NULL, then returns NULL.
 * Otherwise, leaves the Element* untouched and returns error message.
 */
static const char* interpret(const char* str, const nebula_equation::Element*& elt) {
  vector<string> toks;
  string pstr(str);
  replace(pstr.begin(), pstr.end(), '(', ' ');
  replace(pstr.begin(), pstr.end(), ')', ' ');
  istringstream in(pstr);
  string i;
  while (in >> i) toks.push_back(i);

  vector<string>::const_iterator it=toks.begin();
  const nebula_equation::Element* ret=NULL;
  try {
    ret = interpSub(it,vector<string>::const_iterator(toks.end()));
    if (it != toks.end())
      throw "Trailing garbage at end of equation";
  } catch (const char* msg) {
    if (ret) delete ret;
    return msg;
  }

  if (elt) delete elt;
  elt=ret;
  return NULL;
}

/* Shaders */
#ifndef AB_OPENGL_14
struct NebulaBaseUpdateUni {
  vec2 delta, botleft;
  GLuint d, naturals;
  float time, mass, viscosity, fieldWidth, fieldHeight, prubber, vrubber;
};
struct NebulaBaseUpdateVert {
  vec2 position, texCoord;
};
#define VERTEX_TYPE NebulaBaseUpdateVert
#define UNIFORM_TYPE NebulaBaseUpdateUni
DELAY_SHADER(nebula_base_update)
  sizeof(VERTEX_TYPE),
  VATTRIB(position), VATTRIB(texCoord), NULL,
  false, //No transform
  UNIFORM(delta), UNIFORM(botleft),
  UNIFORM(d), UNIFORM(naturals),
  UNIFLOAT(time), UNIFLOAT(mass), UNIFLOAT(viscosity),
  UNIFLOAT(fieldWidth), UNIFLOAT(fieldHeight),
  UNIFLOAT(prubber), UNIFLOAT(vrubber),
  NULL
END_DELAY_SHADER(static nebulaBaseUpdateShader);
#undef UNIFORM_TYPE
#undef VERTEX_TYPE

struct NebulaObjectUpdateUni {
  vec2 delta, botleft;
  GLuint d;
  float mass, forceMul;
};
struct NebulaObjectUpdateVert {
  vec2 position, velocity;
};
#define VERTEX_TYPE NebulaObjectUpdateVert
#define UNIFORM_TYPE NebulaObjectUpdateUni
DELAY_SHADER(nebula_object_update)
  sizeof(VERTEX_TYPE),
  VATTRIB(position), VATTRIB(velocity), NULL,
  false, //No transform
  UNIFORM(delta), UNIFORM(botleft),
  UNIFORM(d), NULL
END_DELAY_SHADER(static nebulaObjectUpdateShader);
DELAY_SHADER_AUX(nebula_object_feedback, &Nebula::attachFeedback)
  sizeof(VERTEX_TYPE),
  VATTRIB(position), VATTRIB(velocity), NULL,
  false,
  UNIFORM(delta), UNIFORM(botleft),
  UNIFORM(d),
  UNIFLOAT(mass), UNIFLOAT(forceMul),
  NULL
END_DELAY_SHADER(static nebulaObjectFeedbackShader);
#undef UNIFORM_TYPE
#undef VERTEX_TYPE

struct NebulaBackgroundDrawUni {
  vec3 colour;
  GLuint d;
};
#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE NebulaBackgroundDrawUni
DELAY_SHADER(nebula_back_draw)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFORM(colour), UNIFORM(d), NULL
END_DELAY_SHADER(static nebulaBackgroundShader);

DELAY_SHADER(nebula_front_draw)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFORM(colour), NULL
END_DELAY_SHADER(static nebulaForegroundShader);
#undef UNIFORM_TYPE
#undef VERTEX_TYPE
#endif /* AB_OPENGL_14 */

Nebula::Nebula(GameObject* go, GameField* gf, float r, float g, float b, float v, float d)
: baseFlowX(NULL), baseFlowY(NULL), basePressure(NULL),
  pointsFront(&points0), pointsBack(&points1), pointsBLX(0), pointsBLY(0),
  colourR(r), colourG(g), colourB(b), glareR(0), glareG(0), glareB(0),
  viscosity(v), density(d),
  prubber(1/10000.0f), vrubber(1/10000.0f), forceMul(1),
  reference(go), field(gf),
  //Begin with update locked so the new thread doesn't constantly spin
  updateLock(SDL_CreateSemaphore(0)), gpuLock(SDL_CreateSemaphore(1)),
  hasUpdateLock(true), hasGPULock(false),
  lastUpdateTime(0), updateTimeAccum(0), alive(true),
  frontBuffer(&buffer0), backBuffer(&buffer1)
{
  if (!headless) {
    #ifndef AB_OPENGL_14
    glGenFramebuffers(1, &buffer0.fbo);
    glGenTextures(1, &buffer0.tex);
    glGenFramebuffers(1, &buffer1.fbo);
    glGenTextures(1, &buffer1.tex);
    glGenTextures(1, &naturalsTex);
    glBindTexture(GL_TEXTURE_2D, buffer0.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, simSideSz, simSideSz, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, buffer1.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, simSideSz, simSideSz, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    feedbackBuffer = newVBO();
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffer);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, MAX_FEEDBACK_OBJECTS*sizeof(vec2), NULL, GL_STREAM_READ);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
    #endif /* AB_OPENGL_14 */
  }

  setFlowEquation(new nebula_equation::Constant(0), new nebula_equation::Constant(0), false); //Don't reset here
  setPressureEquation(new nebula_equation::Constant(0.5), true); //But do reset now

  thread = SDL_CreateThread(&Nebula::run, this);
  field->effects=this;
}

Nebula::~Nebula() {
  alive=false;
  if (hasUpdateLock) SDL_SemPost(updateLock);
  if (hasGPULock)    SDL_SemPost(gpuLock);

  SDL_WaitThread(thread, NULL);

  SDL_DestroySemaphore(updateLock);
  SDL_DestroySemaphore(gpuLock);
  delete baseFlowX;
  delete baseFlowY;
  delete basePressure;
  if (!headless) {
    #ifndef AB_OPENGL_14
    glDeleteFramebuffers(1, &buffer0.fbo);
    glDeleteTextures(1, &buffer0.tex);
    glDeleteFramebuffers(1, &buffer1.fbo);
    glDeleteTextures(1, &buffer1.tex);
    glDeleteTextures(1, &naturalsTex);
    glDeleteBuffers(1, &feedbackBuffer);
    #endif /* AB_OPENGL_14 */
  }
}

const char* Nebula::setFlowEquation(const nebula_equation::Element* xe,
                                    const nebula_equation::Element* ye, bool reset) {
  if (baseFlowX) delete baseFlowX;
  baseFlowX=xe;
  if (baseFlowY) delete baseFlowY;
  baseFlowY=ye;

  if (reset) {
    //Update all points
    for (unsigned y=0; y<simSideSz; ++y) for (unsigned x=0; x<simSideSz; ++x) {
      float xf = (pointsBLX+x)/pointsPerScreen/(float)field->width;
      float yf = (pointsBLY+y)/pointsPerScreen/(float)field->height;
      points0[y*simSideSz+x].vx = points1[y*simSideSz+x].vx = baseFlowX->eval(xf,yf);
      points0[y*simSideSz+x].vy = points1[y*simSideSz+x].vy = baseFlowY->eval(xf,yf);
    }
    for (unsigned y=0; y<naturalGridSz; ++y) for (unsigned x=0; x<naturalGridSz; ++x) {
      float xf = x/(float)naturalGridSz;
      float yf = y/(float)naturalGridSz;
      naturals[y*naturalGridSz*3+x*3+1] = baseFlowX->eval(xf,yf);
      naturals[y*naturalGridSz*3+x*3+2] = baseFlowY->eval(xf,yf);
    }
    blitTextures();
  }
  return NULL;
}

const char* Nebula::setFlowEquation(const char* xes, const char* yes, bool reset) {
  const nebula_equation::Element* xe=NULL, *ye=NULL;
  if (const char* err = interpret(xes, xe)) return err;
  if (const char* err = interpret(yes, ye)) {
    delete xe;
    return err;
  }
  return setFlowEquation(xe,ye,reset);
}

const char* Nebula::setPressureEquation(const nebula_equation::Element* pe, bool reset) {
  if (basePressure) delete basePressure;
  basePressure = pe;

  if (reset) {
    //Update all points
    for (unsigned y=0; y<simSideSz; ++y) for (unsigned x=0; x<simSideSz; ++x) {
      float xf = (pointsBLX+x)/pointsPerScreen/(float)field->width;
      float yf = (pointsBLY+y)/pointsPerScreen/(float)field->height;
      points0[y*simSideSz+x].p = points1[y*simSideSz+x].p = basePressure->eval(xf,yf);
    }
    for (unsigned y=0; y<naturalGridSz; ++y) for (unsigned x=0; x<naturalGridSz; ++x)
      naturals[y*naturalGridSz*3+x*3+0] = basePressure->eval(x/(float)naturalGridSz, y/(float)naturalGridSz);

    blitTextures();
  }

  return NULL;
}

const char* Nebula::setPressureEquation(const char* pes, bool reset) {
  const nebula_equation::Element* pe=NULL;
  if (const char* err = interpret(pes,pe)) return err;
  return setPressureEquation(pe,reset);
}

void Nebula::setPressureResetTime(float t) noth {
  prubber = 1/t;
}

float Nebula::getPressureResetTime() const noth {
  return 1/prubber;
}

void Nebula::setVelocityResetTime(float t) noth {
  vrubber = 1/t;
}

float Nebula::getVelocityResetTime() const noth {
  return 1/vrubber;
}

void Nebula::draw() noth {
  //Release update lock if we hold it, then get the GPU
  if (hasUpdateLock) SDL_SemPost(updateLock);
  if (!hasGPULock) SDL_SemWait(gpuLock);
  hasGPULock=true;
  hasUpdateLock=false;

  #if defined(AB_OPENGL_14) || defined(AB_OPENGL_21)
  glClearColor(colourR+glareR, colourG+glareG, colourB+glareB, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  return;
  #else /* AB_OPENGL_14 */

  BEGINGP("Nebula background")

  //Make the texture linearly interpolated
  glBindTexture(GL_TEXTURE_2D, frontBuffer->tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //Draw
  mPush();
  mTrans(pointsBLX/(float)pointsPerScreen, pointsBLY/(float)pointsPerScreen);
  mUScale(numSimScreens);
  square_graphic::bind();
  NebulaBackgroundDrawUni uni = { {{colourR+glareR, colourG+glareG, colourB+glareB}}, 0 };
  nebulaBackgroundShader->activate(&uni);
  square_graphic::draw();
  mPop();
  //Restore to no interpolation
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  ENDGP
  #endif /* AB_OPENGL_14 */
}

void Nebula::postDraw() noth {
  #if defined(AB_OPENGL_14) || defined(AB_OPENGL_21)
  return;
  #else /* AB_OPENGL_14 */
  BEGINGP("Nebula foreground")
  static GLuint vao=0, vbo;
  if (!vao) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
  }
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  //Calculate and send data
  shader::VertTexc vertices[4];
  float cx = (reference? reference->getX() : 0)+0.5f, cy = (reference? reference->getY() : 0)+0.5f;
  for (unsigned i=0; i<4; ++i) {
    unsigned ix;
    switch (i) { case 0: case 1: ix=i; break; case 2: ix=3; break; case 3: ix=2; break; }
    vertices[ix].vertex = Vec4(screenCorners[i].first, screenCorners[i].second, 0, 1);
    vertices[ix].texCoord = Vec2(cx-screenCorners[i].first, cy-screenCorners[i].second);
  }
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
  nebulaForegroundShader->setupVBO();
  NebulaBackgroundDrawUni uni = { {{colourR+glareR, colourG+glareG, colourB+glareB}}, 0 };
  nebulaForegroundShader->activate(&uni);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  ENDGP
  #endif /* AB_OPENGL_14 */
}

void Nebula::update(float et) noth {
  //Release GPU lock if we hold it, then get update lock
  if (hasGPULock) SDL_SemPost(gpuLock);
  if (hasUpdateLock) SDL_SemPost(updateLock);
  hasUpdateLock=false;
  if (!ENABLE_THREADING) runOnce();
  if (!hasUpdateLock) SDL_SemWait(updateLock);
  hasGPULock=false;
  hasUpdateLock=true;

  float glareMul = pow(GLARE_FALL_SEC, et/1000.0f);
  glareR*=glareMul;
  glareG*=glareMul;
  glareB*=glareMul;

  lastUpdateTime = et;

  //All other work done in other thread
}

void Nebula::updateReference(GameObject* ref, bool reset) noth {
  if (!hasUpdateLock) SDL_SemWait(updateLock);
  reference = ref;
  if (reset) {
    //Just leverage the other functions to do this
    const nebula_equation::Element* oldFX = baseFlowX, *oldFY = baseFlowY, *oldP = basePressure;
    baseFlowX = baseFlowY = basePressure = NULL;
    setPressureEquation(oldP, false);
    setFlowEquation(oldFX, oldFY, true);
  }
  if (!hasUpdateLock) SDL_SemPost(updateLock);
}

void Nebula::explode(Explosion* ex) noth {
  if (!reference) return;
  assert(hasUpdateLock);
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
  if (glareR > 100)
    cout << "High glareR: rx=" << reference->getX() << ", ry=" << reference->getY()
         << ", ex=" << reference->getX() << ", ey=" << reference->getY() << endl;

}

int Nebula::run(void* that) {
  if (ENABLE_THREADING)
    ((Nebula*)that)->run_impl();
  return 0;
}

void Nebula::run_impl() {
  while (alive) runOnce();
}

void Nebula::runOnce() {
  SDL_SemWait(updateLock);
  //OK, clear to copy object information
  #if !defined(AB_OPENGL_14) && !defined(AB_OPENGL_21)
  signed oldPointsBLX = pointsBLX, oldPointsBLY = pointsBLY;
  #endif /* GL32 version */
  if (reference && headless) {
    pointsBLX = (signed)(reference->getX()*pointsPerScreen) - (signed)simSideSz/2;
    pointsBLY = (signed)(reference->getY()*pointsPerScreen) - (signed)simSideSz/2;
  }
  objectInfo.clear();
  float minx = pointsBLX/(float)pointsPerScreen, miny = pointsBLY/(float)pointsPerScreen;
  float maxx = minx+numSimScreens, maxy = miny+numSimScreens;
  for (GameField::iterator it=field->begin(); it != field->end(); ++it) {
    if ((*it)->nebulaInteraction
    &&  (*it)->getX() >= minx && (*it)->getX() <= maxx
    &&  (*it)->getY() >= miny && (*it)->getY() <= maxy) {
      ObjectInfo i;
      i.obj = *it;
      i.elts = i.obj->getNebulaResistanceElements();
      i.len = i.obj->getNumNebulaResistanceElements();
      objectInfo.push_back(i);
    }
  }

  updateTimeAccum += lastUpdateTime;

  SDL_SemPost(updateLock);

  if (!alive) return;
  bool swapBuffers = false;

  if (!headless) {
    SDL_SemWait(gpuLock);
    BEGINGP("Nebula base update")
    #if !defined(AB_OPENGL_14) && !defined(AB_OPENGL_21)
    //Perform base update
    while (updateTimeAccum >= UPDATE_STEP) {
      swapBuffers = true;
      if (reference) {
        pointsBLX = (signed)(reference->getX()*pointsPerScreen) - (signed)simSideSz/2;
        pointsBLY = (signed)(reference->getY()*pointsPerScreen) - (signed)simSideSz/2;
      }
      updateTimeAccum -= UPDATE_STEP;
      //Set the framebuffer up
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frontBuffer->fbo);
      glViewport(0,0,simSideSz,simSideSz);
      glBindTexture(GL_TEXTURE_2D, frontBuffer->tex);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frontBuffer->tex, 0);
      switch (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)) {
        #define ERR(name) case name: cerr << "FATAL: Unable to set framebuffer: " #name << endl; exit(EXIT_PLATFORM_ERROR)
        ERR(GL_FRAMEBUFFER_UNDEFINED);
        ERR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
        ERR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
        ERR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
        ERR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
        ERR(GL_FRAMEBUFFER_UNSUPPORTED);
        ERR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
        case GL_FRAMEBUFFER_COMPLETE: break; //OK
        default:
          cerr << "Unknown error setting framebuffer!" << endl;
          exit(EXIT_PLATFORM_ERROR);
        #undef ERR
      }

      static GLuint vao=0, vbo;
      if (!vao) {
        vao=newVAO();
        glBindVertexArray(vao);
        vbo=newVBO();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        NebulaBaseUpdateVert vertices[4] = {
          {{{-1,-1}}, {{0,0}}},
          {{{+1,-1}}, {{1,0}}},
          {{{-1,+1}}, {{0,1}}},
          {{{+1,+1}}, {{1,1}}},
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        nebulaBaseUpdateShader->setupVBO();
      }

      //Set the textures up
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, backBuffer->tex);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, naturalsTex);
      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glActiveTexture(GL_TEXTURE0);

      NebulaBaseUpdateUni uni = {
        //Delta is in texture coordinates
        {{(pointsBLX-oldPointsBLX)/(float)simSideSz, (pointsBLY-oldPointsBLY)/(float)simSideSz}},
        //Bottom-left is in natural texture coordinates
        {{pointsBLX/(float)pointsPerScreen/field->width,
          pointsBLY/(float)pointsPerScreen/field->height }},
        1, 2,
        UPDATE_STEP, density, viscosity,
        field->width, field->height,
        prubber, vrubber
      };

      //OK, draw it
      nebulaBaseUpdateShader->activate(&uni);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      //Keep the framebuffer set up, but restore the secondary texture
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, 0);
      glActiveTexture(GL_TEXTURE0);

      if (updateTimeAccum >= UPDATE_STEP) {
        swap(frontBuffer, backBuffer);
        oldPointsBLX=pointsBLX;
        oldPointsBLY=pointsBLY;
      }

      ENDGP
    }

    //Object updates
    //This section is NOT reentrant (it uses static vectors
    //to improve performance).
    {
      BEGINGP("Nebula object update")
      static vector<NebulaObjectUpdateVert> vertices;
      //Parallel vector to store the original NebulaResistanceElements and GameObjects
      static vector<pair<const NebulaResistanceElement*,GameObject*> > vinfos;
      vertices.clear();
      vinfos.clear();
      for (unsigned i=0; i<objectInfo.size(); ++i) for (unsigned j=0; j<objectInfo[i].len; ++j) {
        float x = objectInfo[i].elts[j].px * pointsPerScreen;
        float y = objectInfo[i].elts[j].py * pointsPerScreen;
        if (x < pointsBLX || y < pointsBLY || x > pointsBLX+simSideSz || y > pointsBLY+simSideSz)
          continue;
        //Usable vertex
        NebulaObjectUpdateVert v = { {{objectInfo[i].elts[j].px, objectInfo[i].elts[j].py}},
                                     {{objectInfo[i].elts[j].vx, objectInfo[i].elts[j].vy}} };
        vertices.push_back(v);
        vinfos.push_back(make_pair(&objectInfo[i].elts[j], objectInfo[i].obj));
      }

      if (!vertices.empty()) {
        static GLuint vao=0, vbo;
        if (!vao) {
          vao=newVAO();
          glBindVertexArray(vao);
          vbo=newVBO();
          glBindBuffer(GL_ARRAY_BUFFER, vbo);
          nebulaObjectUpdateShader->setupVBO();
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(NebulaObjectUpdateVert)*vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);
        NebulaObjectUpdateUni uni = {
          //Delta is in texture coordinates
          {{(pointsBLX-oldPointsBLX)/(float)simSideSz, (pointsBLY-oldPointsBLY)/(float)simSideSz}},
          //Bottom-left is in world coordinates
          {{pointsBLX/(float)pointsPerScreen, pointsBLY/(float)pointsPerScreen}},
          1,
          density, forceMul
        };
        /* My nVidia card disables rasterisation when doing transform feedback,
         * so we have to do this twice; disable rasterisation explicitly anyway
         * for maximum compatibility.
         */
        glUseProgram(0);
        nebulaObjectFeedbackShader->activate(&uni);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBuffer);
        glBeginTransformFeedback(GL_POINTS);
        glEnable(GL_RASTERIZER_DISCARD);
        glDrawArrays(GL_POINTS, 0, vertices.size());
        glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);
        nebulaObjectUpdateShader->activate(&uni);
        glDrawArrays(GL_POINTS, 0, vertices.size());

        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffer);
        const vec2* feedback = (const vec2*)glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
        if (!feedback) {
          cerr << "Unable to map feed back buffer, ignoring nebular physics..." << endl;
        } else {
          unsigned count = (vertices.size() > MAX_FEEDBACK_OBJECTS? MAX_FEEDBACK_OBJECTS : vertices.size());
          //Reset all object forces
          for (unsigned i=0; i<count; ++i)
            vinfos[i].second->nebulaFrictionX = vinfos[i].second->nebulaFrictionY =
                vinfos[i].second->nebulaTorsion = 0;

          for (unsigned i=0; i<count; ++i) {
            vec2 force = feedback[i];
            //Due to the way we handle the nebula, the force returned by the GPU
            //is proportional to the last update time. Divide by that value
            //to undo that
            vinfos[i].second->nebulaFrictionX += force[0]/lastUpdateTime;
            vinfos[i].second->nebulaFrictionY += force[1]/lastUpdateTime;
            vinfos[i].second->nebulaTorsion += (force & Vec2(vinfos[i].first->cx, vinfos[i].first->cy))/lastUpdateTime;
          }

          if (GL_TRUE != glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER)) {
            cerr << "glUnMapBuffer returned GL_FALSE, discarding feedback information" << endl;
            //Reset all physics infos to 0, as the feedback information we got was
            //probably corrupted
            for (unsigned i=0; i<count; ++i)
              vinfos[i].second->nebulaFrictionX = vinfos[i].second->nebulaFrictionY =
                  vinfos[i].second->nebulaTorsion = 0;
          }
        }

        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
      }

      //We're done with the framebuffer
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, 0);
      glViewport(0,0,screenW,screenH);
      //glFinish();
      ENDGP
      #endif /* AB_OPENGL_14 */
    }

    SDL_SemPost(gpuLock);
  }

  //Swap front and back buffers
  swap(pointsFront, pointsBack);
  if (swapBuffers) swap(frontBuffer, backBuffer);
}

void Nebula::blitTextures() noth {
  if (headless) return;
  if (!hasGPULock) SDL_SemWait(gpuLock);

  #if !defined(AB_OPENGL_14) && !defined(AB_OPENGL_21)

  glBindTexture(GL_TEXTURE_2D, naturalsTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, naturalGridSz, naturalGridSz, 0, GL_RGB, GL_FLOAT, naturals);
  glBindTexture(GL_TEXTURE_2D, buffer0.tex);
  static float buffdat[simSideSz*simSideSz*3];
  for (unsigned i=0; i<simSideSz*simSideSz; ++i) {
    buffdat[i*3+0]=points0[i].p;
    buffdat[i*3+1]=points0[i].vx;
    buffdat[i*3+2]=points0[i].vy;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, simSideSz, simSideSz, 0, GL_RGB, GL_FLOAT, buffdat);
  glBindTexture(GL_TEXTURE_2D, buffer1.tex);
  for (unsigned i=0; i<simSideSz*simSideSz; ++i) {
    buffdat[i*3+0]=points1[i].p;
    buffdat[i*3+1]=points1[i].vx;
    buffdat[i*3+2]=points1[i].vy;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, simSideSz, simSideSz, 0, GL_RGB, GL_FLOAT, buffdat);

  #endif /* AB_OPENGL_14 */

  if (!hasGPULock) SDL_SemPost(gpuLock);
}

void Nebula::attachFeedback(GLuint shader) {
  #if !defined(AB_OPENGL_14) && !defined(AB_OPENGL_21)
  static const char* names[] = { "force" };
  glTransformFeedbackVaryings(shader, 1, names, GL_SEPARATE_ATTRIBS);
  #endif /* AB_OPENGL_14 */
}
