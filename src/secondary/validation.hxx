/**
 * @file
 * @author Jason Lingle
 * @brief Contains the getValidationCode() function.
 *
 * This file contains an interface to a proprietary module of Abendstern
 * which verifies that:
 * (a) The build is official, and
 * (b) The Tcl code associated with networking has not been modified
 *
 * This exists solely to make it more difficult to modify the client and
 * cheat in Internet play. (It is my opinion that anyone who knows enough
 * to bypass or reverse-engineer this could also perform other modifications
 * without detection, including cheating without tripping formerly-planned
 * autodetection.)
 *
 * If this is the pure Open-Source version of Abendstern which does not include
 * that module, define AB_PURE_OPEN, and a validation function which always
 * returns 0 will be used instead.
 *
 * (Note that due to certain reasons, proprietary validation logic is not
 * currently included in any case, and the null versions are always used
 * instead.)
 */

/*
 * validation.hxx
 *
 *  Created on: 28.09.2011
 *      Author: jason
 */

#ifndef VALIDATION_HXX_
#define VALIDATION_HXX_

//#ifndef AB_PURE_OPEN
#if 0

/**
 * Runs a validation check against the current version of Abendstern with the
 * given challenge. The return values can be retrieved with getValidationResultA()
 * and getValidationResultB().
 */
extern void performValidation(int,int,int,int);
extern int getValidationResultA();
extern int getValidationResultB();

#else

static inline void performValidation(int,int,int,int) { }
static inline int getValidationResultA() { return 0; }
static inline int getValidationResultB() { return 0; }

#endif

#endif /* VALIDATION_HXX_ */
