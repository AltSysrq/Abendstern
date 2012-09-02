/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/background/planet.hxx
 */

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_image.h>

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <vector>

#include "planet.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/globals.hxx"
#include "src/graphics/imgload.hxx" //for scaleImage
#include "src/camera/effects_handler.hxx"
#include "explosion.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

#define TEXW 64
#define TEXH 256

struct PlanetVertex {
  vec2 vertex;
  vec2 texCoord;
  float isLeft;
};

struct PlanetUniform {
  GLuint dayTex, nightTex;
  vec4 glareColour;
  float dayNightLeft, dayNightRight;
};

#define VERTEX_TYPE PlanetVertex
#define UNIFORM_TYPE PlanetUniform
#ifndef AB_OPENGL_14
DELAY_SHADER(planet)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), VFLOAT(isLeft), NULL,
  true,
  UNIFORM(dayTex), UNIFORM(nightTex),
  UNIFORM(glareColour),
  UNIFLOAT(dayNightLeft), UNIFLOAT(dayNightRight),
  NULL
END_DELAY_SHADER(static planetShader);

//Places tile (x,y) into the given texture
static void planet_cutTile(SDL_Surface* source, int x, int y, GLuint tex) {
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    Uint32 rmask=0xff000000,
           gmask=0x00ff0000,
           bmask=0x0000ff00,
           amask=0x000000ff;
  #else
    Uint32 rmask=0x000000ff,
           gmask=0x0000ff00,
           bmask=0x00ff0000,
           amask=0xff000000;
  #endif
  SDL_Surface* slice=SDL_CreateRGBSurface(SDL_SWSURFACE, TEXW, TEXH, 32,
                                          rmask, gmask, bmask, amask);
  SDL_Rect rect = { (Sint16)(x*TEXW), (Sint16)(y*TEXH), TEXW, TEXH };
  SDL_BlitSurface(source, &rect, slice, NULL);
  slice=scaleImage(slice);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, slice->w, slice->h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, slice->pixels);
  glBindTexture(GL_TEXTURE_2D, 0);
  SDL_FreeSurface(slice);
}
#endif /* AB_OPENGL_14 */

Planet::Planet(GameObject* ref, GameField* _field,
               const char* day, const char* night, float revolutionTime, float orbitTime,
               float height, float _twilight)
: reference(ref), field(_field), twilight(_twilight),
  rev(rand()/(float)RAND_MAX), orbit(rand()/(float)RAND_MAX),
  vrev(1/revolutionTime), vorbit(1/orbitTime),
  glareR(0), glareG(0), glareB(0)
{
  //We can't use imgload because we need to manually split the textures up

  if (!headless) {
#ifndef AB_OPENGL_14
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);


    SDL_Surface* dayimg, *nightimg;
    if (! (dayimg=IMG_Load(day))) {
      cerr << "Error loading planet day image: " << IMG_GetError() << endl;
      exit(EXIT_MALFORMED_DATA);
    } if (! (nightimg=IMG_Load(night))) {
      cerr << "Error loading planet night image: " << IMG_GetError() << endl;
      exit(EXIT_MALFORMED_DATA);
    }

    if (dayimg->w != nightimg->w || dayimg->h != nightimg->h) {
      cerr << "Planet night and day images are different sizes" << endl;
      exit(EXIT_MALFORMED_DATA);
    }

    numTexturesWide = dayimg->w/TEXW;
    numTexturesHigh = dayimg->h/TEXH;

    tileSize = height/numTexturesHigh;
    float tileH = tileSize, tileW = tileSize*TEXW/(float)TEXH;
    parallax = height/(field->height)/2/sqrt(2.0f);

    PlanetVertex vertices[4] = {
      { {{0,0}}, {{0,1}}, 1 },
      { {{tileW,0}}, {{1,1}}, 0 },
      { {{0,tileH}}, {{0,0}}, 1 },
      { {{tileW,tileH}}, {{1,0}}, 0 },
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    planetShader->setupVBO();

    SDL_SetAlpha(dayimg, 0, 0);
    SDL_SetAlpha(nightimg, 0, 0);

    int numTextures=numTexturesHigh*numTexturesWide*2;
    textures = new GLuint[numTextures];
    glGenTextures(numTextures, textures);
    for (int x=0; x<numTexturesWide; ++x) {
      for (int y=0; y<numTexturesHigh; ++y) {
        GLuint daytex   = textures[(x+y*numTexturesWide)*2],
               nighttex = textures[(x+y*numTexturesWide)*2 + 1];
        planet_cutTile(dayimg, x, y, daytex);
        planet_cutTile(nightimg, x, y, nighttex);
      }
    }

    SDL_FreeSurface(dayimg);
    SDL_FreeSurface(nightimg);
#endif /* AB_OPENGL_14 */
  }

  field->effects=this;
}

Planet::~Planet() {
  field->effects=&nullEffectsHandler;

#ifndef AB_OPENGL_14
  if (!headless) {
    glDeleteTextures(numTexturesWide*numTexturesHigh*2, textures);
    delete[] textures;
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
#endif /* AB_OPENGL_14 */
}

void Planet::draw() noth {
#ifndef AB_OPENGL_14
  BEGINGP("Planet")

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  float paraX = (cameraCX-field->width/2)*parallax, paraY=(cameraCY-field->height/2)*parallax;

  float dayBegin = orbit + twilight/2;
  float dayEnd = orbit + 0.5f - twilight/2;
  float nightBegin = orbit + 0.5f + twilight/2;
  float nightEnd = orbit + 1.0f - twilight/2;

  float tileH = tileSize, tileW = tileSize*TEXW/(float)TEXH;
  for (int x=0; x<numTexturesWide; ++x) for (int y=0; y<numTexturesHigh; ++y) {
    float off0=x/(float)numTexturesWide, off1=(x-1)%numTexturesWide / (float)numTexturesWide;
    while (off0 < dayBegin) off0 += 1.0f;
    while (off1 < dayBegin) off1 += 1.0f;
    float blendl = (off0 < dayEnd? 1.0f
                   :off0 < nightBegin? 1.0f - (off0-dayEnd)/twilight
                   :off0 < nightEnd? 0
                   :/*off0<dayBegin2*/ (off0-nightEnd)/twilight);
    float blendr = (off1 < dayEnd? 1.0f
                   :off1 < nightBegin? 1.0f - (off1-dayEnd)/twilight
                   :off1 < nightEnd? 0
                   :/*off1<dayBegin2*/ (off1-nightEnd)/twilight);

    mPush();
    if (reference)
      mTrans(reference->getX(),
             reference->getY());
    mUScale(1/cameraZoom);
    mTrans(fmod(x+orbit*numTexturesWide, numTexturesWide)*tileW-tileW*numTexturesWide/2 - paraX,
           y*tileH-tileH*numTexturesHigh/2 - paraY);
    int tx = x;//numTexturesWide-x-1;
    int ty = numTexturesHigh-y-1;
    glBindTexture(GL_TEXTURE_2D, textures[(tx+ty*numTexturesWide)*2]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[(tx+ty*numTexturesWide)*2+1]);
    glActiveTexture(GL_TEXTURE0);
    PlanetUniform uni = { 0, 1, {{glareR,glareG,glareB,1}}, blendr, blendl };
    planetShader->activate(&uni);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    mPop();
  }

  ENDGP
#endif /* AB_OPENGL_14 */
}

#define GLARE_FALL_SEC 0.04f
void Planet::update(float time) noth {
  float glareMul = pow(GLARE_FALL_SEC, time/1000.0f);
  glareR*=glareMul;
  glareG*=glareMul;
  glareB*=glareMul;

  rev+=vrev*time;
  orbit+=vorbit*time;

  if (rev>1) rev-=1;
  if (rev<0) rev+=1;
  if (orbit>1) orbit-=1;
  if (orbit<0) orbit+=1;
}

void Planet::explode(Explosion* ex) noth {
  if (!reference) return;
  float dx=reference->getX()-ex->getX(), dy=reference->getY()-ex->getY();
  float dist=sqrt(dx*dx + dy*dy);
  float mul=ex->effectsDensity*ex->getSize()/dist;
  float r = ex->getColourR()*mul,
        g = ex->getColourG()*mul,
        b = ex->getColourB()*mul;
  glareR+=r;
  glareG+=g;
  glareB+=b;
}

void Planet::updateReference(GameObject* o, bool reset) noth {
  if (reset) glareR=glareG=glareB=0;
  reference=o;
}
