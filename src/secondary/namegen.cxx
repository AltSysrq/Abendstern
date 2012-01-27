/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/secondary/namegen.hxx
 */

/*
 * namegen.cxx
 *
 *  Created on: 22.07.2011
 *      Author: jason
 */

#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cerrno>

#include <libconfig.h++>

#include "namegen.hxx"
#include "src/exit_conditions.hxx"

using namespace std;
using namespace libconfig;

typedef unsigned uchar;
typedef vector<uchar> ustring;

/**
 * Pseudodecodes a UTF-8 character into a uchar and returns it.
 * The input will be incremented to be one past the read char.
 */
static uchar utf8pd(const unsigned char*& in) {
  uchar r = *in++;
  unsigned shift = 8;
  if ((r & 0xC0) == 0xC0) do {
    r |= ((uchar)*in++) << shift;
    shift += 8;
  } while (((*in) & 0x80) && !((*in) & 0x40));
  return r;
}

/**
 * Encodes a pseudodecoded character back into UTF-8.
 * @param ch The uchar to encode
 * @param out The byte array to write into;
 *   the pointer will be incremented one past the written char.
 */
static void utf8en(uchar ch, unsigned char*& out) {
  do {
    *out++ = (ch & 0xFF);
    ch >>= 8;
    ch &= 0x00FFFFFF;
  } while (ch);
}

struct region {
  string name;
  vector<ustring> strings;
  unsigned weight;
};
static vector<region> regions;

void namegenLoad() noth {
  Config conf;
  conf.readFile("data/namegen/list.dat");
  const Setting& lst(conf.getRoot());
  for (unsigned i=0; i<(unsigned)lst.getLength(); ++i) {
    region reg;
    reg.name = (const char*)lst[i].getName();
    reg.weight = lst[i]["weight"];

    ifstream in((const char*)lst[i]["filename"]);
    if (!in) {
      cerr << "Error opening " << (const char*)lst[i]["filename"] << ": " << strerror(errno) << endl;
      exit(EXIT_MALFORMED_CONFIG);
    }

    string line;
    while (getline(in, line)) {
      if (line.empty()) continue;
      ustring entry;
      entry.reserve(line.size());
      const unsigned char* input = (const unsigned char*)line.c_str();
      while (*input) entry.push_back(utf8pd(input));
      reg.strings.push_back(entry);
    }

    regions.push_back(reg);
  }
}

string namegenGet(const char* regname) {
  if (!regname) regname = "";
  region* reg = NULL;
  unsigned totalWeight=0;

  //See if a region with this name exists
  //While doing that, accumulate weight
  for (unsigned i=0; i<regions.size() && !reg; ++i)
    if (regions[i].name == regname)
      reg = &regions[i];
    else
      totalWeight += regions[i].weight;

  if (!reg) {
    //Select at random
    signed w = rand()%totalWeight;
    unsigned ix=0;
    while (w >= 0) w -= regions[ix++].weight;

    reg = &regions[ix-1]; //ix is off by one since we postincremented
  }

  ustring str;
  //Initialise with a random name
  unsigned initName = rand()%reg->strings.size();
  str.push_back(reg->strings[initName][0]);
  str.push_back(reg->strings[initName][1]);
  //Apply Dissociated Press (limit size, though)
  while (str.size() < 16) {
    uchar c0 = str[str.size()-2], c1 = str[str.size()-1];
    vector<uchar> nextMatches;
    for (unsigned i=0; i<reg->strings.size(); ++i)
      for (unsigned j=0; j<reg->strings[i].size()-1; ++j)
        if (reg->strings[i][j] == c0 && reg->strings[i][j+1] == c1)
          nextMatches.push_back(j < reg->strings[i].size()-2? reg->strings[i][j+2] : 0);

    if (nextMatches.empty())
      //This is a problem, just exit the loop
      break;

    //Select a random match
    uchar nxt = nextMatches[rand() % nextMatches.size()];
    if (nxt)
      str.push_back(nxt);
    else
      break;
  }

  //Transcode back to UTF-8
  unsigned char tmp[5] = {0};
  string ret;
  ret.reserve(str.size());
  for (unsigned i=0; i<str.size(); ++i) {
    unsigned char* dst = (unsigned char*)tmp;
    utf8en(str[i], dst);
    *dst=0;
    ret += (char*)tmp;
  }

  return ret;
}
