/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implementation of src/fasttrig.hxx.
 */
#include <cmath>
#include <cstdlib>

#include "globals.hxx"
#include "fasttrig.hxx"
using namespace std;

float sintable[TRIGTABLE_SZ];
int randtableA[RANDTABLE_SZ];
int randtableB[RANDTABLE_SZ];

void initTable() {
  for (int i=0; i<TRIGTABLE_SZ; ++i)
    sintable[i]=sin(i*pi*2.0f/(float)TRIGTABLE_SZ);
  for (int i=0; i<RANDTABLE_SZ; ++i) {
    randtableA[i]=rand();
    randtableB[i]=rand();
  }
}
