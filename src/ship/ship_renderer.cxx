/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/ship_renderer.hxx
 */

/*
 * ship_renderer.cxx
 *
 *  Created on: 30.01.2011
 *      Author: jason
 */

//Only compile in non-1.4 mode
#ifndef AB_OPENGL_14

#include <map>
#include <vector>
#include <new>
#include <cstring>
#include <typeinfo>

#include <GL/gl.h>

#include "ship_renderer.hxx"
#include "render_quad.hxx"
#include "ship.hxx"
#include "cell/cell.hxx"
#include "src/graphics/vec.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/shader.hxx"
#include "src/graphics/shader_loader.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/control/human_controller.hxx"
#include "ship_pallet.hxx"
#include "src/globals.hxx"
#include "src/secondary/confreg.hxx"

using namespace std;

struct ShipShaderUniform {
  GLuint mainTex, damTex;
  vec3 shipColour;
  float highColour[128*3];
  float cloak;
};

/* In order to preserve framerate, we limit the number of
 * new RenderQuads we make ready per frame. When the prepare
 * count becomes greater than the prepare limit, no non-high-
 * priority quads will be prepared. A high-priority quad is
 * one in a renderer that has been informed of a major physical
 * change (cell removal). When a renderer draws, it sets
 * the count to 0 if the current clock is different from the
 * reset clock, then updates the reset clock.
 */
static unsigned quadPrepareCount, quadPrepareLimit;
static unsigned long quadPrepareClock;

#define VERTEX_TYPE shader::VertTexc
#define UNIFORM_TYPE ShipShaderUniform
DELAY_SHADER(ship)
  sizeof(VERTEX_TYPE),
  VATTRIB(vertex), VATTRIB(texCoord), NULL,
  true,
  UNIFORM(mainTex), UNIFORM(damTex), UNIFORM(shipColour),
  UNIFLOATA(highColour), UNIFLOAT(cloak), NULL
END_DELAY_SHADER(static shipShader);

ShipRenderer::ShipRenderer(Ship* s)
: owner(s), quads(NULL), isHighPriority(false)
{
  setPallet(P_G_CYANDARK0, 0, 0xE0/255.0f, 0xE0/255.0f);
  setPallet(P_G_CYANDARK1, 0, 0xC0/255.0f, 0xC0/255.0f);
  setPallet(P_G_CYANDARK2, 0, 0xA0/255.0f, 0xA0/255.0f);
  setPallet(P_G_CYANDARK3, 0, 0x80/255.0f, 0x80/255.0f);
  setPallet(P_G_BLUEDARK0, 0, 0, 0xE0/255.0f);
  setPallet(P_G_BLUEDARK1, 0, 0, 0xC0/255.0f);
  setPallet(P_G_BLUEDARK2, 0, 0, 0xA0/255.0f);
  setPallet(P_G_BLUEDARK3, 0, 0, 0x80/255.0f);
  setPallet(P_G_YELLORNG0, 1, 1, 0);
  setPallet(P_G_YELLORNG1, 1, 0.75f, 0);
  setPallet(P_G_YELLORNG2, 1, 0.5f, 0);
  setPallet(P_G_YELLORNG2, 1, 0.25f, 0);
  setPallet(P_G_GREYLGHT0, 0x33/255.0f, 0x33/255.0f, 0x33/255.0f);
  setPallet(P_G_GREYLGHT1, 0x66/255.0f, 0x66/255.0f, 0x66/255.0f);
  setPallet(P_G_GREYLGHT2, 0x99/255.0f, 0x99/255.0f, 0x99/255.0f);
  setPallet(P_G_GREYLGHT3, 0xCC/255.0f, 0xCC/255.0f, 0xCC/255.0f);
  setPallet(P_GREEN,       0, 1, 0);
  setPallet(P_DARKYELLOW,  0xCC/255.0f, 0xCC/255.0f, 0);
  setPallet(P_DARKBLUECYAN, 0, 0.5f, 1);
  setPallet(P_WHITE, 1, 1, 1);

  quadPrepareLimit = conf["conf"]["graphics"]["ship_draw_rate"];
}

ShipRenderer::~ShipRenderer() {
  free();
}

void ShipRenderer::free() noth {
  if (quads) {
    for (unsigned i=0; i<nPerSide*nPerSide; ++i)
      delete quads[i];
    delete[] quads;
  }
  quads=NULL;
  isHighPriority=false;
  cell2Quad.clear();
}

void ShipRenderer::cellDamage(Cell* cell) noth {
  map<Cell*,vector<RenderQuad*> >::iterator it=cell2Quad.find(cell);
  if (it != cell2Quad.end()) {
    const vector<RenderQuad*>& quads((*it).second);
    for (unsigned i=0; i<quads.size(); ++i)
      quads[i]->cellDamaged();
  }
}

void ShipRenderer::cellRemoved(Cell* cell) noth {
  map<Cell*,vector<RenderQuad*> >::iterator it=cell2Quad.find(cell);
  if (it != cell2Quad.end()) {
    const vector<RenderQuad*>& quads((*it).second);
    for (unsigned i=0; i<quads.size(); ++i)
      quads[i]->physicalChange(cell);

    cell2Quad.erase(it);
  }

  isHighPriority=true;
}

void ShipRenderer::cellChanged(Cell* cell) noth {
  map<Cell*,vector<RenderQuad*> >::iterator it=cell2Quad.find(cell);
  if (it != cell2Quad.end()) {
    const vector<RenderQuad*>& quads((*it).second);
    for (unsigned i=0; i<quads.size(); ++i)
      quads[i]->physicalChange(NULL);
  }
}

void ShipRenderer::draw() noth {
  if (quadPrepareClock != gameClock) {
    quadPrepareCount=0;
    quadPrepareClock = gameClock;
  }

  if (!quads || !--framesUntilDetailCheck) {
    if (!quads || detailLevel != (unsigned)conf["conf"]["graphics"]["detail_level"]) {
      if (quads) isHighPriority=true;

      free();
      owner->physicsRequire(PHYS_SHIP_COORDS_BIT | PHYS_CELL_LOCATION_PROPERTIES_BIT);
      gradius=owner->getBoundingSquareHalfEdge()+STD_CELL_SZ*2;
      bridgeX=owner->cells[0]->getX();
      bridgeY=owner->cells[0]->getY();

      detailLevel=conf["conf"]["graphics"]["detail_level"];
      nPerSide = (unsigned)ceil(gradius*2/STD_CELL_SZ/detailLevel);

      quads = new RenderQuad*[nPerSide*nPerSide];
      for (unsigned y=0; y<nPerSide; ++y) for (unsigned x=0; x<nPerSide; ++x)
        quads[y*nPerSide + x] = new RenderQuad(detailLevel, x, y);
      //Find which cells belong to which quads
      //First, teleport ship to standard location
      float* tmpDat=owner->temporaryZero();
      owner->teleport(gradius, gradius, 0);
      for (unsigned i=0; i<nPerSide*nPerSide; ++i)
        for (unsigned j=0; j<owner->cells.size(); ++j)
          if (quads[i]->doesCellIntersect(owner->cells[j]))
            cell2Quad[owner->cells[j]].push_back(quads[i]);
      owner->restoreFromZero(tmpDat);
    }

    /* Wait a while before seeing if we must change detail level. */
    framesUntilDetailCheck = 0x80 + (rand()&0xFF);
  }

  //Are all the quads ready?
  bool ready=true;
  for (unsigned i=0; i<nPerSide*nPerSide && ready; ++i)
    ready = quads[i]->isReady();

  if (!ready) {
    BEGINGP("Ship rendering")
    //Modify transforms so that ship appears in standard area
    mPush(matrix_stack::view);
    mId(matrix_stack::view);
    mPush();
    mId();

    //Now draw any that aren't ready
    for (unsigned i=0; i<nPerSide*nPerSide; ++i)
      if (!quads[i]->isReady() && (isHighPriority || quadPrepareCount < quadPrepareLimit)) {
        ++quadPrepareCount;
        quads[i]->makeReady(gradius-detailLevel*STD_CELL_SZ/2-(owner->cells[0]->getX()-bridgeX),
                            gradius-detailLevel*STD_CELL_SZ/2-(owner->cells[0]->getY()-bridgeY));
      }

    //Restore matrices
    mPop();
    mPop(matrix_stack::view);
    ENDGP
  }

  //OK, we may now proceed with drawing
  ShipShaderUniform uni;
  uni.mainTex=0;
  uni.damTex=1;
  uni.shipColour=Vec3(owner->getColourR(), owner->getColourG(), owner->getColourB());
  if (owner->isCloaked()) {
    if (owner->controller && typeid(*owner->controller) == typeid(HumanController))
      //Don't become fully transparent so that the player can still see his ship
      uni.cloak = 1.0f - 0.80*owner->getStealthCounter() / (float)STEALTH_COUNTER_MAX;
    else
      uni.cloak = 1.0f - owner->getStealthCounter() / (float)STEALTH_COUNTER_MAX;
  } else {
    uni.cloak = 1.0f;
  }
  if (uni.cloak > 0) {
    memcpy(uni.highColour, pallet, sizeof(pallet));
    mTrans(-gradius+(owner->cells[0]->getX()-bridgeX),
           -gradius+(owner->cells[0]->getY()-bridgeY));
    shipShader->activate(&uni);
    for (unsigned i=0; i<nPerSide*nPerSide; ++i)
      if (quads[i]->isReady())
        quads[i]->draw();
  }
}

#endif /* AB_OPENGL_14 */
