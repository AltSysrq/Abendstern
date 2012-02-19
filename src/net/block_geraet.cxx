/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.19
 * @brief Implementation of block Geräte base classes
 */

#include <algorithm>
#include <map>
#include <vector>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cassert>

#include <SDL.h>

#include "block_geraet.hxx"
#include "io.hxx"

#include "src/exit_conditions.hxx"

using namespace std;

InputBlockGeraet::InputBlockGeraet(unsigned sz, AsyncAckGeraet* aag)
: AAGReceiver(aag),
  lastSeq(0), state(sz, 0), dirty(sz, true)
{
}

void InputBlockGeraet::receiveAccepted(NetworkConnection::seq_t,
                                       const byte* data, unsigned len)
throw () {
  //Ignore if too short
  if (len <= sizeof(block_geraet_seq)+2*sizeof(Uint16)) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring block packet of invalid size " << len << endl;
    #endif
    return;
  }

  const byte* end = data+len;

  block_geraet_seq seq;
  Uint16 fragix, nfrags;
  io::read(data, seq);
  io::read(data, fragix);
  io::read(data, nfrags);

  if (seq <= lastSeq) return; //Obsolete

  if (fragix >= nfrags) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring block update fragment " << fragix << '/'
         << nfrags << endl;
    #endif
    return;
  }

  //Create the fragment series if it does not yet exist
  if (!frags.count(seq)) {
    frags.insert(make_pair(seq, vector<vector<byte> >(nfrags)));
  }

  vector<vector<byte> >& frg = frags[seq];
  if (frg.size() != nfrags) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring incorrect nfrags in block update" << endl;
    cerr << "Reported: " << nfrags << "; actual: " << frg.size() << endl;
    #endif
    return;
  }

  if (!frg[fragix].empty()) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring duplicate block update fragment" << endl;
    #endif
    return;
  }

  frg[fragix].assign(data, end);

  //See if this mutation is complete, and accumulate total size.
  //If it isn't, we're done for now.
  unsigned size = 0;
  for (unsigned i=0; i<nfrags; ++i)
    if (frg[i].empty())
      return;
    else
      size += frg[i].size();

  //Ready to reassemble
  vector<byte> assembled(size);
  for (unsigned i=0; i<nfrags; ++i)
    assembled.insert(assembled.end(),
                     frg[i].begin(), frg[i].end());

  data = &assembled[0];
  end = data+size;

  //Set seq information
  lastSeq = seq;
  //Remove any obsoleted mutations, including this one
  while (frags.begin()->first <= seq)
    frags.erase(frags.begin());

  //Parse data
  unsigned ix;
  while (data < end) {
    byte head = *data++;
    unsigned off, len;

    #define STOPLEN(l) if (data+(l) >= end) goto endloop

    //Interpret offset/size information
    if (head & 1) {
      off = ((head >> 1) & 0xF);
      len = ((head >> 5) & 0x7);
    } else {
      switch ((head >> 1) & 0x7) {
        Uint16 u16;
        unsigned u0, u1, u2;
        case 0:
          //4-4
          STOPLEN(1);
          off = (*data & 0xF);
          len = ((*data >> 4) & 0xF);
          ++data;
          break;

        case 1:
          //6-2
          STOPLEN(1);
          off = (*data & 0x3F);
          len = ((*data >> 6) & 0x3);
          ++data;
          break;

        case 2:
          //8-8
          STOPLEN(2);
          off = *data++;
          len = *data++;
          break;

        case 3:
          //12-4
          STOPLEN(2);
          u0 = *data++;
          u1 = *data++;
          off = u0 | ((u1 & 0xF) << 8);
          len = ((u1 >> 4) & 0xF);
          break;

        case 4:
          //16-8
          STOPLEN(3);
          io::read(data, u16);
          off = u16;
          len = *data++;
          break;

        case 5:
          //20-4
          STOPLEN(3);
          u0 = *data++;
          u1 = *data++;
          u2 = *data++;
          off = u0 | (u1 << 8) | ((u2 & 0xF) << 16);
          len = ((u2 >> 4) & 0xF);
          break;

        case 6:
          //24-8
          STOPLEN(4);
          io::read24(data, off);
          len = *data++;
          break;

        case 7:
          //8-4 embedded
          STOPLEN(1);
          len = *data++;
          off = ((head >> 4) & 0xF);
          break;

        default:
          assert(false);
          exit(EXIT_PROGRAM_BUG);
      }
    }

    ix += off;
    STOPLEN(len);
    if (ix+len > state.size()) break; //No longer in valid bounds

    //OK, apply changes
    memcpy(&state[ix], data, len);
    //Mark dirty
    fill_n(dirty.begin()+ix, len, true);
    //Update indices
    ix += len;
    data += len;
  }

  endloop:
  //Notify subclass of modification
  modified();
}
