/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of explosion_pool.hxx
 */

#include <GL/gl.h>
#include <vector>
#include <deque>
#include <cmath>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <cstring>

#include "explosion_pool.hxx"
#include "explosion.hxx"
#include "old_style_explosion.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"
#include "src/fasttrig.hxx"
#include "src/sim/game_field.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/camera/effects_handler.hxx"

//Our debugging with FPE exceptions leads to problems with
//the use of uninitialised values in the arrays, so we'll need
//to temporarily disable the exceptions during update
#if !defined(WIN32) && defined(DEBUG)
#include </usr/include/fenv.h>
#endif

using namespace std;

#define MAX_EXPLOSIONS_DRAWN 3000

static int numExDrawn;

static GLuint vao, vbo;

struct ExplosionUniform {
  float ex, ey;
  float elapsedTime, sizeAt1Sec, density;
  unsigned id;
  vec4 colour;
};

#ifndef AB_OPENGL_14
namespace shader {
  #define VERTEX_TYPE shader::Vert2
  #define UNIFORM_TYPE ::ExplosionUniform
  DELAY_SHADER(simpleExplosion)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), NULL,
    true,
    UNIFLOAT(ex), UNIFLOAT(ey),
    UNIFLOAT(elapsedTime), UNIFLOAT(sizeAt1Sec), UNIFLOAT(density),
    UNIFORM(id), UNIFORM(colour), NULL
  END_DELAY_SHADER(static simpleEx);

  DELAY_SHADER(sparkExplosion)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), NULL,
    true,
    UNIFLOAT(ex), UNIFLOAT(ey),
    UNIFLOAT(elapsedTime), UNIFLOAT(sizeAt1Sec), UNIFLOAT(density),
    UNIFORM(id), UNIFORM(colour), NULL
  END_DELAY_SHADER(static spark);

  DELAY_SHADER(bigSparkExplosion)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), NULL,
    true,
    UNIFLOAT(ex), UNIFLOAT(ey),
    UNIFLOAT(elapsedTime), UNIFLOAT(sizeAt1Sec), UNIFLOAT(density),
    UNIFORM(id), UNIFORM(colour), NULL
  END_DELAY_SHADER(static bigSpark);

  DELAY_SHADER(sparkleExplosion)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), NULL,
    true,
    UNIFLOAT(ex), UNIFLOAT(ey),
    UNIFLOAT(elapsedTime), UNIFLOAT(sizeAt1Sec), UNIFLOAT(density),
    UNIFORM(id), UNIFORM(colour), NULL
  END_DELAY_SHADER(static sparkle);

  DELAY_SHADER(incursionExplosion)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), NULL,
    true,
    UNIFLOAT(ex), UNIFLOAT(ey),
    UNIFLOAT(elapsedTime), UNIFLOAT(sizeAt1Sec), UNIFLOAT(density),
    UNIFORM(id), UNIFORM(colour), NULL
  END_DELAY_SHADER(static incursion);

  DELAY_SHADER(flameExplosion)
    sizeof(VERTEX_TYPE),
    VATTRIB(vertex), NULL,
    true,
    UNIFLOAT(ex), UNIFLOAT(ey),
    UNIFLOAT(elapsedTime), UNIFLOAT(sizeAt1Sec), UNIFLOAT(density),
    UNIFORM(id), UNIFORM(colour), NULL
  END_DELAY_SHADER(static flame);
}

static const Shader* shaders[6];
#endif /* AB_OPENGL_14 */

ExplosionPoolSegment::ExplosionPoolSegment() {
  freeCount=EXPLOSIONS_PER_SEGMENT;
  for (unsigned int i=0; i<sizeof(usage)/sizeof(unsigned int); ++i)
    usage[i]=0;
  //Set everything in the time stripe to infinity
  float infinity=INFINITY;
  for (unsigned i=0; i<lenof(timeStripe); i+=2) timeStripe[i]=infinity;
}

ExplosionPoolSegment::~ExplosionPoolSegment() {
}

inline void ExplosionPoolSegment::update(float et) noth {
  //First, update sizes
  //We don't care whether anything is in use,
  //because it'd take longer to check than
  //to just do the calculations
  float* ptr=coordStripe;
  float* end=ptr+EXPLOSIONS_PER_SEGMENT*EX_VLEN;
  while (ptr<end) {
    *ptr += (*(ptr+1))*et;
    ptr += 2;
  }

  //Now go through times.
  //If any time is too high, unset the usage bit
  unsigned int* usage=this->usage;
  unsigned int mask=1;
  GLfloat (*colours)[4]=this->colours;
  ptr=timeStripe;
  end=ptr+EXPLOSIONS_PER_SEGMENT*EX_TLEN;
  while (ptr<end) {
    //This line triggers a warning from Valgrind
    //That is OK, this was designed to crunch
    //through uninitialized values
    *ptr-=et;
    if (*ptr < 0) {
      //The time has expired, mark free
      *usage &= ~mask;
      ++freeCount;
      //Make ptr infinite
      float a=0;
      *ptr = 1/a;
    }
    (*colours)[3] = (*ptr / *(ptr+1));
    mask<<=1;
    //If we are 0, we have gone through
    //all 64 or 32 bits
    if (mask==0) {
      ++usage;
      mask=1;
    }
    ptr+=2;
    ++colours;
  }
}

#define GETX(i) coordStripe[EX_X_OFF + ((i)*EX_VLEN)]
#define GETY(i) coordStripe[EX_Y_OFF + ((i)*EX_VLEN)]
#define GETET(i) timeStripe[EX_ET_OFF + ((i)*EX_TLEN)]
#define GETLT(i) timeStripe[EX_LT_OFF + ((i)*EX_TLEN)]

ExplosionPool::ExplosionPool()
{
  static bool hasVAO=false;
#ifndef AB_OPENGL_14
  if (!hasVAO && !headless) {
    static const shader::Vert2 vertices[4] = {
      {{{-1,+1}}},
      {{{-1,-1}}},
      {{{+1,+1}}},
      {{{+1,-1}}},
    };

    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::spark->setupVBO();

    shaders[Explosion::Simple   ] = shader::simpleEx .operator->();
    shaders[Explosion::Spark    ] = shader::spark    .operator->();
    shaders[Explosion::BigSpark ] = shader::bigSpark .operator->();
    shaders[Explosion::Sparkle  ] = shader::sparkle  .operator->();
    shaders[Explosion::Incursion] = shader::incursion.operator->();
    shaders[Explosion::Flame    ] = shader::flame    .operator->();
  }
#endif /* AB_OPENGL_14 */
}

void ExplosionPoolSegment::draw() noth {
#ifndef AB_OPENGL_14
  ExplosionUniform uni;
  //We can afford to be a bit more sane because this is called less often
  //and the majority of the grunt work is handled by GL
  for (unsigned int i=0; i<EXPLOSIONS_PER_SEGMENT; ++i) {
    if (usage[i/sizeof(unsigned int)/8] & (1 << (i%(sizeof(unsigned int)*8)))) {
      if (++numExDrawn >= MAX_EXPLOSIONS_DRAWN && (rand()&1)) continue;
      uni.ex = coordStripe[i*EX_VLEN + EX_X_OFF];
      uni.ey = coordStripe[i*EX_VLEN + EX_Y_OFF];
      uni.elapsedTime = (timeStripe[i*EX_TLEN + EX_LT_OFF] - timeStripe[i*EX_TLEN + EX_ET_OFF])/1000;
      uni.sizeAt1Sec = sizes[i];
      uni.density = densities[i];
      GLfloat red=colours[i][0], green=colours[i][1], blue=colours[i][2], alpha=colours[i][3];
      //Somewhat realistic (looking) blackbody radiation:
      //Blue and green fade out earlier
      blue *= (alpha>0.25? (alpha-0.25f)/0.75f : 0);
      green *= alpha;
      uni.colour = Vec4(red,green,blue,alpha);
      uni.id=i+1;
      shaders[types[i]]->activate(&uni);
      #ifndef AB_OPENGL_21
      glDrawArrays(GL_POINTS, 0, 1); //The actual vertex is ignored
      #else
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      #endif
    }
  }
#endif /* AB_OPENGL_14 */
}

ExplosionPool::~ExplosionPool() {
  for (unsigned int i=0; i<segments.size(); ++i)
    delete segments[i];
}

void ExplosionPool::update(float et) noth {
  #if defined(DEBUG) && !defined(WIN32)
  assert(-1 != fedisableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif

  for (unsigned i=0; i<hungryExplosions.size(); ++i)
    if (!--hungryExplosions[i].framesLeft)
      hungryExplosions.erase(hungryExplosions.begin() + (i--));

  for (unsigned int i=0; i<segments.size(); ++i)
    segments[i]->update(et);

  ++segmentGCCounter;
  //Disabled for now. It may have been causeing performance issues, since
  //the buffers are naturally rolled through.
  if (segmentGCCounter==0 && false) {
    //Kill any segment that is completely empty, other than the first
    for (unsigned int i=1; i<segments.size(); ++i) {
      for (unsigned int j=0; j<sizeof(segments[i]->usage)/sizeof(unsigned int); ++j)
        if (segments[i]->usage[j]) goto used;

      //Unused if we get here
      delete segments[i];
      segments.erase(segments.begin() + i);
      --i;
      continue;

      used:;
    }
  }

  #if defined(DEBUG) && !defined(WIN32)
  assert(-1 != feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif
}

void ExplosionPool::draw() noth {
#ifndef AB_OPENGL_14
  BEGINGP("ExplosionPool")
  numExDrawn=0;
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  for (unsigned int i=0; i<segments.size(); ++i)
    segments[i]->draw();
  ENDGP
#endif /* AB_OPENGL_14 */
}

/* I put these in separate functions for profiling.
 * Anyway, even in debug mode where they are not inlined,
 * three function calls shouldn't be too much of a penalty.
 */
#ifdef PROFILE
#define inline
#endif
inline void ExplosionPool::findFreeSegment(ExplosionPoolSegment*& segment, int& block, int& bit, int& index,
                                           vector<ExplosionPoolSegment*>& segments) noth {
  for (unsigned int i=0; i<segments.size(); ++i) {
    if (segments[i]->freeCount) {
      for (unsigned int j=0; j<sizeof(segments[i]->usage)/sizeof(unsigned int); ++j) {
        if (~segments[i]->usage[j]) {
          segment=segments[i];
          block=j;
          for (unsigned int k=0; k<sizeof(unsigned int)*8; ++k) {
            if (! (segment->usage[block] & (1 << k)) ) {
              bit=k;
              index=bit + block*sizeof(unsigned int)*8;
              return;
            }
          }
        }
      }
    }
  }

  //Not found, allocate new segment
  segments.push_back(segment=new ExplosionPoolSegment);
  block=0;
  index=0;
  bit=0;
}

void ExplosionPool::add(Explosion* ex) noth {
#ifdef AB_OPENGL_14
  delete ex;
  return;
#else
  if (headless) {
    delete ex;
    return;
  }

  ex->getField()->effects->explode(ex);

  if (ex->type == Explosion::Invisible) {
    delete ex;
    return;
  }

  //See if it is to be dropped
  for (unsigned i=0; i<hungryExplosions.size(); ++i) {
    if (hungryExplosions[i].type == ex->type
    &&  hungryExplosions[i].minX <= ex->x
    &&  hungryExplosions[i].maxX >  ex->x
    &&  hungryExplosions[i].minY <= ex->y
    &&  hungryExplosions[i].maxY >  ex->y) {
      //Drop
      delete ex;
      return;
    }
  }

  if (ex->hungry) {
    HungryExplosion he = {ex->type, ex->x-0.05f, ex->x+0.05f, ex->y-0.05f, ex->y+0.5f, 8};
    hungryExplosions.push_back(he);
  }

  /* Possibly redirect to old-style explosion */
  if (ex->type == Explosion::BigSpark
  &&  conf["shaders"][isLSDModeForced()? "lsd" :
                      (const char*)conf["conf"]["graphics"]["shader_profile"]]["redirect_big_spark_explosion"]) {
    ex->field->add(new OldStyleExplosion(ex));
    delete ex;
    return;
  }

  //Find a free item
  ExplosionPoolSegment* segment;
  int block, bit, index;
  findFreeSegment(segment, block, bit, index, segments);

  //Copy parms
  float* coords=segment->coordStripe+index*EX_VLEN;
  *(coords+EX_X_OFF)=ex->x;
  *(coords+EX_Y_OFF)=ex->y;
  *(coords+EX_VX_OFF)=ex->vx;
  *(coords+EX_VY_OFF)=ex->vy;
  float* times=segment->timeStripe+index*EX_TLEN;
  *(times+EX_ET_OFF)=ex->lifetime;
  *(times+EX_LT_OFF)=ex->lifetime;

  segment->colours[index][0]=ex->colour[0];
  segment->colours[index][1]=ex->colour[1];
  segment->colours[index][2]=ex->colour[2];
  segment->colours[index][3]=1; //full alpha at beginning

  segment->types[index] = ex->type;
  segment->sizes[index] = ex->sizeAt1Sec;
  segment->densities[index] = ex->density;

  //Mark used
  segment->usage[block] |= (1 << bit);
  --segment->freeCount;

  //We don't want the Explosion anymore
  delete ex;
#endif /* !defined(AB_OPENGL_14) */
}

