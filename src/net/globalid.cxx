/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/net/globalid.hxx
 */

/*
 * globalid.cxx
 *
 *  Created on: 09.12.2011
 *      Author: jason
 */

#include <string>
#include <cstdio>
#include <cstring>

#include "globalid.hxx"

using namespace std;

string GlobalID::toString() const {
  static char dst[1024];
  if (ipv == IPv4)
    sprintf(dst, "%d.%d.%d.%d:%d/%d.%d.%d.%d:%d",
            (unsigned)ia4[0], (unsigned)ia4[1], (unsigned)ia4[2],
            (unsigned)ia4[3], (unsigned)iport,
            (unsigned)la4[0], (unsigned)la4[1], (unsigned)la4[2],
            (unsigned)la4[3], (unsigned)lport);
  else
    sprintf(dst, "%x.%x.%x.%x.%x.%x.%x.%x:%d/"
                 "%x.%x.%x.%x.%x.%x.%x.%x:%d",
            (unsigned)ia6[0], (unsigned)ia6[1],
            (unsigned)ia6[2], (unsigned)ia6[3],
            (unsigned)ia6[4], (unsigned)ia6[5],
            (unsigned)ia6[6], (unsigned)ia6[7],
            (unsigned)iport,
            (unsigned)la6[0], (unsigned)la6[1],
            (unsigned)la6[2], (unsigned)la6[3],
            (unsigned)la6[4], (unsigned)la6[5],
            (unsigned)la6[6], (unsigned)la6[7],
            (unsigned)lport);

  return string(dst);
}
