/**
 * @file
 * @author Jason Lingle
 *
 * @brief Provides facilities for fast trigonometry and random numbers.
 *
 * These definitions should be used in cases where very large
 * numbers of the appropriate calls must be made quickly, but
 * accuracy is not a concern.
 */

#ifndef FASTTRIG_HXX_
#define FASTTRIG_HXX_

#include "globals.hxx"
#include "opto_flags.hxx"

/* fasttrig is simply a trigonometry table to speed up things that need
 * cos() and sin() a lot, but do not depend upon accuracy; for example,
 * explosions.
 * It also now encompases a random table for faster random number
 * generation (ie, in explosions) where so many randoms are needed
 * that the CPU will end up caching the entire table. The random is
 * based on the XOR of two tables, the second advancing once per
 * revolution of the other (thus allowing for 4 billion values instead
 * of only 65536).
 */

/** Defines the size of the trigonometry tables.
 * This must be a power of two.
 */
#define TRIGTABLE_SZ 16384
/** Defines the bitmask used for performing modulus operations against TRIGTABLE_SZ. */
#define TRIGTABLE_MASK (TRIGTABLE_SZ-1)
/** Defines the offset within the sine table to find the values for cosine. */
#define TRIGTABLE_COS_OFF TRIGTABLE_SZ/4
/** Defines the size of the random number tables.
 * This must be a power of two.
 */
#define RANDTABLE_SZ 65536
/** Defines the bitmask used for performing modulus operations against RANDTABLE_SZ. */
#define RANDTABLE_MASK (RANDTABLE_SZ-1)
/** Table of values for sine. Runs from 0 to 2*pi. */
extern float sintable[TRIGTABLE_SZ];
/** Table of random integers. Used in conjunction with the
 * other table to produce random values quickly.
 */
extern int randtableA[RANDTABLE_SZ];
/** Table of random integers. Used in conjunction with the
 * other table to produce random values quickly.
 */
extern int randtableB[RANDTABLE_SZ];

/* FCOS() and FSIN() macros expect integers, [0, TRIGTABLE_SZ). */
/** Looks the specified cosine value up. The given integer should be
 * within the range [0, TRIGTABLE_SZ), where 0=0.0 and TRIGTABLE_SZ=2π.
 */
#define FCOS(x) sintable[((x)+TRIGTABLE_COS_OFF)&TRIGTABLE_MASK]
/** Looks the specified sine value up. The given integer should be
 * within the range [0, TRIGTABLE_SZ), where 0=0.0 and TRIGTABLE_SZ=2π.
 */
#define FSIN(x) sintable[x]

/* Same as above, but work with any integers. */
/** Looks the specified cosine value up. The given integer does not
 * have to be in the valid range.
 */
#define FCOSS(x) FCOS(x)
/** Looks the specified sine value up. The given integer does not
 * have to be in the valid range.
 */
#define FSINS(x) FSIN((x)&TRIGTABLE_MASK)

/* Functions that take normal floats. */
/** Performs a fast lookup of the cosine of the specified value. */
static inline float fcos(float x) noth {
  return sintable[((int)(x/2/pi*TRIGTABLE_SZ)+TRIGTABLE_COS_OFF) & TRIGTABLE_MASK];
}

/** Performs a fast lookup of the sine of the specified value. */
static inline float fsin(float x) noth {
  return sintable[((int)(x/2/pi*TRIGTABLE_SZ)) & TRIGTABLE_MASK];
}

/** Generates a random integer.
 * @return A pseudorandom integer with the same properties as one returned by std::rand().
 */
static inline int frand() noth {
  static int a=0, b=0;
  if (!(a=(a+1)&RANDTABLE_MASK)) b=(b+1)&RANDTABLE_MASK;
  return randtableA[a]^randtableB[b];
}

/** Initialises the trig and rand tables.
 * This MUST be called before any other function (or definition) in
 * this file.
 */
void initTable();
#endif /*FASTTRIG_HXX_*/
