/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ShipRenderer class
 */

/*
 * ship_renderer.hxx
 *
 *  Created on: 29.01.2011
 *      Author: jason
 */

#ifndef SHIP_RENDERER_HXX_
#define SHIP_RENDERER_HXX_

#include <vector>
#include <map>

#include "src/opto_flags.hxx"
#include "ship_pallet.hxx"

class Ship;
class Cell;
class RenderQuad;

/** The ShipRenderer handles the actual rendering of Ships.
 * It maintains an array of RenderQuads, and can ajust the
 * detail level on the fly.
 *
 * The RenderQuads are arranged in a grid of squares. Each
 * square is detailLevel*STD_CELL_SZ on a side; squares which
 * correspond to no cells are never rendered.
 *
 * This class is separate from Ship to prevent ship.cxx free
 * of graphics code, since it is easier if that file contains
 * only physics code.
 *
 * The separation also makes freeing of resources easier.
 *
 * This class assumes that no visible cells will ever be added
 * to a Ship after the first time it is drawn. Specifically,
 * cells not before encountered are silently ignored.
 */
class ShipRenderer {
  Ship* const owner;
  /* The GRadius of the ship at the time we
   * created the creation of the RenderQuad array.
   */
  float gradius;
  /* We keep track of the relative location of the
   * root cell, setting these at the same time
   * as gradius. This allows us to compensate for
   * changes in the centre location.
   */
  float bridgeX, bridgeY;

  /* An array of RenderQuads.
   */
  RenderQuad** quads;
  unsigned nPerSide;

  /* Map to allow quickly find which RenderQuads are affected by
   * a given Cell.
   */
  std::map<Cell*, std::vector<RenderQuad*> > cell2Quad;

  /* The effective detail level of this particular renderer. */
  unsigned detailLevel;

  /* The number of calls to draw() until we check to see if we
   * need to change effective detail levels.
   */
  unsigned framesUntilDetailCheck;

  bool isHighPriority;

  public:
  /** The colour pallet, for colours 0x80--0xFF, to use for
   * the ship. This is specific to the renderer, so fragments
   * need not refresh it every frame.
   *
   * All static colours are initialized correctly. Others may be
   * left uninitialized.
   */
  float pallet[P_DYNAMIC_SZ][3];
  /** Sets the pallet at the given index to the given colour. */
  inline void setPallet(unsigned ix, float r, float g, float b) {
    pallet[ix-P_DYNAMIC_BEGIN][0]=r;
    pallet[ix-P_DYNAMIC_BEGIN][1]=g;
    pallet[ix-P_DYNAMIC_BEGIN][2]=b;
  }

  /** Create the renderer. The ship does not need to be fully
   * constructed at this point. No resources are allocated
   * by the constructor.
   */
  ShipRenderer(Ship*);
  ~ShipRenderer();

  /** Draw the ship, making any alterations necessary.
   * The Ship must have translated the current matrix
   * so that [0,0] is the centre and such that rotation
   * is correct. Additionally, the colour pallet must
   * have been filled.
   */
  void draw() noth;

  /** Free all resources held by the renderer. */
  void free() noth;

  /** Specify that the given cell has been damaged, and therefore
   * its damage texture must be refreshed.
   */
  void cellDamage(Cell*) noth;

  /** Specify that the given cell has been removed from the ship. */
  void cellRemoved(Cell*) noth;

  /** Specify that something has physically changed for the given
   * Cell that is not normal damage or deletion.
   */
  void cellChanged(Cell*) noth;
};

#endif /* SHIP_RENDERER_HXX_ */
