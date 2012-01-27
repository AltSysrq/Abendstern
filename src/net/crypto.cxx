/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/net/crypto.hxx
 */

/*
 * crypto.cxx
 *
 *  Created on: 20.08.2011
 *      Author: jason
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <cerrno>

#ifdef WIN32
#include <windows.h>
#include <mpir.h>
//We must link against advapi32.lib

static HCRYPTPROV cryptoProvider;

#else /* WIN32 */

#include <gmp.h>

static FILE* urandom;

#endif /* !WIN32 */

#include "src/exit_conditions.hxx"

using namespace std;

static mpz_t modulusVal;

void crypto_init(const char* modstr) {
  mpz_init_set_str(modulusVal, modstr, 16);

  #ifdef WIN32
    if (!CryptAcquireContextW(&cryptoProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
      cerr << "FATAL: CryptAcquireContextW failed" << endl;
      exit(EXIT_PLATFORM_ERROR);
    }
  #else /* WIN32 */
    urandom = fopen("/dev/urandom", "rb");
    if (!urandom) {
      cerr << "FATAL: Could not open /dev/urandom: " << strerror(errno) << endl;
      exit(EXIT_PLATFORM_ERROR);
    }
  #endif /* !WIN32 */
}

const char* crypto_rand() {
  static unsigned char dat[128/8];
  static char str[2*sizeof(dat)+1];
  #ifdef WIN32
    if (!CryptGenRandom(cryptoProvider, sizeof(dat), dat)) {
      cerr << "FATAL: CryptGenRandom failed" << endl;
      exit(EXIT_PLATFORM_ERROR);
    }
  #else /* WIN32 */
    if (1 != fread(dat, sizeof(dat), 1, urandom)) {
      cerr << "FATAL: Error reading from /dev/urandom: " << strerror(errno) << endl;
      exit(EXIT_PLATFORM_ERROR);
    }
  #endif /* !WIN32 */

  const char* digits = "0123456789ABCDEF";
  for (unsigned i=0; i<sizeof(dat); ++i) {
    str[2*i+0] = digits[dat[i]>>4];
    str[2*i+1] = digits[dat[i]&15];
  }
  str[sizeof(str)-1] = 0;
  return str;
}

const char* crypto_powm(const char* basestr, const char* powerstr) {
  //Since we'll let mpz_get_str allocate its own string, we need to
  //free it on another call, so keep track of that here
  static char* resultstr = NULL;
  mpz_t base, power;
  mpz_init_set_str(base, basestr, 16);
  mpz_init_set_str(power, powerstr, 16);

  if (resultstr) free(resultstr);

  mpz_powm(base, base, power, modulusVal);
  resultstr = mpz_get_str(NULL, 16, base);

  mpz_clear(base);
  mpz_clear(power);
  return resultstr;
}
