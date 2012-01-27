/**
 * @file
 * @author Jason Lingle
 * @brief Contains functions used for cryptography and some mathematical
 * functions that have no application elsewhere.
 *
 * These are used for securing the Abendstern Network protocol. They are
 * generally only called by Tcl.
 */

/*
 * crypto.hxx
 *
 *  Created on: 20.08.2011
 *      Author: jason
 */

#ifndef CRYPTO_HXX_
#define CRYPTO_HXX_

/** Performs any platform-specific initialisation that must
 * be done to run any other functions in this file.
 *
 * @arg modulus A hexadecimal integer to use as the modulus in
 * the functions that do so.
 */
void crypto_init(const char* modulus);

/** Generates a cryptographically-secure 128-bit integer in
 * a platform-specific manner and returns it as a hexadecimal
 * string withOUT an "0x" prefix.
 *
 * This function is not reentrant.
 */
const char* crypto_rand();

/** Raises the first argument to the power of the second. Both
 * are assumed to be in hexadecimal with no "0x" prefix.
 * The result is modulo the modulus specified in crypto_init(),
 * and is returned as a hexadecimal string with no "0x" prefix.
 *
 * This function is not reentrant.
 */
const char* crypto_powm(const char*,const char*);

#endif /* CRYPTO_HXX_ */
