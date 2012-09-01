/**
 * @file
 * @author Jason Lingle
 * @date 2012.07.22
 * @brief Implementation of ShipImageRenderer.
 */

#include <GL/gl.h>
#include <SDL.h>

#include <functional>
#include <vector>

#include <png++/png.hpp>

#include "src/ship/ship.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/matops.hxx"
#include "src/ship/ship_renderer.hxx"
#include "src/globals.hxx"
#include "ship_image_renderer.hxx"

#define PX_PER_CELL 16

using namespace std;

static bool fless(float a, float b) { return a < b; }
static bool fgreater(float a, float b) { return a > b; }

// Returns the minimum or maximum value of the given dimension in the Ship
static float axisMax(Ship* s, float (Cell::*dim)() const,
                     bool (*compare)(float,float)) {
  float max = 0;
  for (unsigned i = 0; i < s->cells.size(); ++i)
    if (compare((s->cells[i]->*dim)(), max))
      max = (s->cells[i]->*dim)();
  return max;
}

// Moves the Ship on both axes so that the most extreme point is at pixel
// zero on each.
static Ship* relocate(Ship* s) {
  s->teleport(0,0,0);
  float minx = axisMax(s, &Cell::getX, fless) - STD_CELL_SZ;
  float miny = axisMax(s, &Cell::getY, fless) - STD_CELL_SZ;
  s->teleport(minx, miny, 0);
  return s;
}

// Calculates the number of pixels required in the given dimension.
static unsigned calcDim(Ship* s, float (Cell::*dim)() const) {
  float max = axisMax(s, dim, fgreater);
  float min = axisMax(s, dim, fless   );

  float size = max - min + STD_CELL_SZ * 2;
  return (unsigned)(PX_PER_CELL * size / STD_CELL_SZ);
}

static unsigned pow2(unsigned x) {
  unsigned i;
  for (i=1; i < x; i *= 2);
  return i;
}

ShipImageRenderer::ShipImageRenderer(Ship* s)
: ship(relocate(s)), currentCell(0),
  imgW(calcDim(s, &Cell::getX)),
  imgH(calcDim(s, &Cell::getY)),
  imgW2(pow2(imgW)),
  imgH2(pow2(imgH)),
  texture(0)
{
}

ShipImageRenderer::~ShipImageRenderer() {
  if (texture)
    glDeleteTextures(1, &texture);
}

bool ShipImageRenderer::renderNext() {
  #ifdef AB_OPENGL_14
  return false;
  #else
  if (headless)
    return false;

  static bool hasFbo = false;
  static GLuint fbo;
  if (!hasFbo) {
    glGenFramebuffers(1, &fbo);
    hasFbo = true;
  }
  if (!fbo) return false;

  if (!texture) {
    glGenTextures(1, &texture);
    if (!texture) return false;
    glBindTexture(GL_TEXTURE_2D, texture);
    vector<GLubyte> zero(imgW2*imgH2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, imgW2, imgH2, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, &zero[0]);
  }

  if (currentCell >= ship->cells.size())
    return false; //done

  bool status = false;

  //Bind the framebuffer to the texture
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, texture);
  glViewport(0,0,imgW2,imgH2);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, texture, 0);
  switch (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)) {
    #define ERR(name) case name: cerr << "WARN: Unable to set framebuffer: " \
        #name << endl; goto done;
    ERR(GL_FRAMEBUFFER_UNDEFINED);
    ERR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
    ERR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
    ERR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
    ERR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
    ERR(GL_FRAMEBUFFER_UNSUPPORTED);
    ERR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
  case GL_FRAMEBUFFER_COMPLETE: break; //OK
  default:
    cerr << "WARN: Unknown error setting framebuffer!" << endl;
    goto done;
  #undef ERR
  }

  //Draw the next cell
  mPush(matrix_stack::view);
  mId(matrix_stack::view);
  mPush();
  mId();
  mTrans(-1,-1);
  mScale(PX_PER_CELL / STD_CELL_SZ / imgW2 * 2,
         PX_PER_CELL / STD_CELL_SZ / imgH2 * 2);
  mTrans(-ship->getX(),-ship->getY());
  ship->cells[currentCell++]->draw();
  mPop();
  mPop(matrix_stack::view);

  status = true;

  done:
  //Restore the normal parms
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glViewport(0,0,screenW,screenH);

  //Free the texture if we failed
  if (!status) {
    glDeleteTextures(1, &texture);
    texture = 0;
  }

  return status;
  #endif /* AB_OPENGL_14 */
}

bool ShipImageRenderer::save(const char* filename) const {
  if (!texture)
    return false;

  vector<GLubyte> indexed(imgW2*imgH2);
  png::image<png::rgba_pixel> img(imgW,imgH);
  glBindTexture(GL_TEXTURE_2D, texture);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &indexed[0]);

  //Get the colours to use for translation
  ShipRenderer renderer(NULL);
  ship->preparePallet(&renderer);
  //Translate shader input to ARGB
  for (unsigned y = 0; y < imgH; ++y) {
    for (unsigned x = 0; x < imgW; ++x) {
      GLubyte ix = indexed[x + y*imgW2];
      unsigned a, r, g, b;
      if (!ix)
        a = r = g = b = 0; //Transparent
      else if (ix == P_BLACK)
        a = 255, r = g = b = 0; //Black
      else if (ix <= P_BODY_END) {
        float mult = (ix - P_BODY_BEGIN + 1) / (float)P_BODY_SZ * 0.7f + 0.3f;
        a = 255;
        r = (unsigned)(255 * ship->getColourR() * mult);
        g = (unsigned)(255 * ship->getColourG() * mult);
        b = (unsigned)(255 * ship->getColourB() * mult);
      } else {
        ix -= P_DYNAMIC_BEGIN;
        a = 255;
        r = (unsigned)(255*renderer.pallet[ix][0]);
        g = (unsigned)(255*renderer.pallet[ix][1]);
        b = (unsigned)(255*renderer.pallet[ix][2]);
      }

      img[imgH-y-1][x] = png::rgba_pixel(r,g,b,a);
    }
  }

  try {
    img.write(filename);
  } catch (...) {
    return false;
  }
  return true;
}

ShipImageRenderer* ShipImageRenderer::create(Ship* ship) {
  #ifdef AB_OPENGL_14
  return NULL;
  #else
  if (headless)
    return NULL;
  else
    return new ShipImageRenderer(ship);
  #endif
}
