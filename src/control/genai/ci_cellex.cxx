/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_cellex.hxx
 */

/*
 * ci_cellex.cxx
 *
 *  Created on: 30.10.2011
 *      Author: jason
 */

#include "src/ship/cell/cell.hxx"
#include "ci_cellex.hxx"

void cortex_input::cellExExamineCell(const Cell* cell, float* dst) {
  dst[CellExInputOffset::celldamage] = cell->getCurrDamage() / cell->getMaxDamage();
  dst[CellExInputOffset::cxo] = cell->getX();
  dst[CellExInputOffset::cyo] = cell->getY();
  unsigned sidesFree = 0;
  for (unsigned i=0; i<4; ++i)
    if (!cell->neighbours[i]) ++sidesFree;
  dst[CellExInputOffset::cnsidesfree] = sidesFree;
  dst[CellExInputOffset::cbridge] = (cell->usage == CellBridge? 1.0f : 0.0f);
}
