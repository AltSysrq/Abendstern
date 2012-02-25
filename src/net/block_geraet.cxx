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

//Don't have more than this many outstanding synchronous packets
//at one time.
#define MAX_SYNC_OUTPACK 32

InputBlockGeraet::InputBlockGeraet(unsigned sz, AsyncAckGeraet* aag,
                                   DeletionStrategy ds)
: AAGReceiver(aag, ds),
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

  //OK
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
  vector<byte> assembled;
  assembled.reserve(size);
  for (unsigned i=0; i<nfrags; ++i)
    assembled.insert(assembled.end(),
                     frg[i].begin(), frg[i].end());

  data = &assembled[0];
  end = data+size;

  //Set seq information
  lastSeq = seq;
  //Remove any obsoleted mutations, including this one
  while (!frags.empty() && frags.begin()->first <= seq)
    frags.erase(frags.begin());

  //Parse data
  unsigned ix=0;
  while (data < end) {
    byte head = *data++;
    unsigned off, len;

    #define STOPLEN(l) if (data+(l) > end) goto endloop

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
          off = *data++;
          len = ((head >> 4) & 0xF);
          break;

        default:
          assert(false);
          exit(EXIT_PROGRAM_BUG);
      }
    }

    ix += off;
    ++len; //Length is stored as one less than it actually is
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
  #undef STOPLEN
}


OutputBlockGeraet::OutputBlockGeraet(unsigned sz, AsyncAckGeraet* aag,
                                     DeletionStrategy ds)
: AAGSender(aag, ds),
  nextSeq(1),
  old(sz, 0), state(sz, 0),
  dirty(false)
{
}

void OutputBlockGeraet::update(unsigned) throw() {
  //Send new packet if dirty and not waiting in synchronous mode
  if (dirty && syncPending.empty()) {
    vector<byte> packet;
    block_geraet_seq seq = nextSeq++;
    packet.reserve(64);

    //Scan for changes between states
    unsigned index = 0;
    for (unsigned i=0; i < state.size(); ++i) {
      if (old[i] != state[i]) {
        //Found change.
        //Search for continguous changes; only stop if more than one consecutive
        //non-changed byte is encountered.
        //Additionally, note that we cannot encode more than 256 bytes in
        //one chunk.
        unsigned begin = i;
        while (i < state.size() && i-begin < 256
        &&     (old[i] != state[i]
            ||  (i+1 < state.size() && old[i+1] != state[i+1])))
          ++i;

        //i is now one past the last changed byte, or at the maximum length
        unsigned len = i-begin;
        unsigned elen = len-1;

        //Move back one byte since the outer loop will increment i again
        --i;

        //Pick the most compact encoding for offset and length
        unsigned off = begin - index;
        byte head[5];
        unsigned headlen;
        //See newnetworking.txt for the significance of the numbers below.
        if (off < 16 && elen < 8) {
          //Embedded
          head[0] = 1 | (off << 1) | (elen << 5);
          headlen = 1;
        } else if (off < 16 && elen < 16) {
          //4-4
          head[0] = 0;
          head[1] = off | (elen << 4);
          headlen = 2;
        } else if (off < 64 && elen < 4) {
          //6-2
          head[0] = 2;
          head[1] = off | (elen << 6);
          headlen = 2;
        } else if (off < 256 && elen < 16) {
          //8-4 embedded
          head[0] = 14 | (elen << 4);
          head[1] = off;
          headlen = 2;
        } else if (off < 256 && elen < 256) {
          //8-8
          head[0] = 4;
          head[1] = off;
          head[2] = elen;
          headlen = 3;
        } else if (off < 4096 && elen < 16) {
          //12-4
          head[0] = 6;
          head[1] = (off & 0xFF);
          head[2] = (off >> 8) | (elen << 4);
          headlen = 3;
        } else if (off < 65536 && elen < 256) {
          //16-8
          head[0] = 8;
          head[1] = (off & 0xFF);
          head[2] = (off >> 8);
          head[3] = elen;
          headlen = 4;
        } else if (off < (1 << 20) && elen < 16) {
          //20-4
          head[0] = 10;
          head[1] = (off & 0xFF);
          head[2] = ((off >> 8) & 0xFF);
          head[3] = ((off >> 16) | (elen << 4));
          headlen = 4;
        } else {
          //24-8
          head[0] = 12;
          head[1] = (off & 0xFF);
          head[2] = ((off >> 8) & 0xFF);
          head[3] = (off >> 16);
          head[4] = elen;
          headlen = 5;
        }

        //Copy header then data
        packet.insert(packet.end(), head, head+headlen);
        packet.insert(packet.end(),
                      state.begin()+begin, state.begin()+begin+len);

        //Advance index
        index = begin+len;
      }
    }

    //If nothing changed, unset dirty and finish
    if (packet.empty()) {
      dirty = false;
      return;
    }

    //The maximum amount of data that each packet can hold
    static const unsigned capacity =
        256-NetworkConnection::headerSize -
        sizeof(block_geraet_seq)-2*sizeof(Uint16);
    Uint16 nfrags = (packet.size()+capacity-1)/capacity;
    //Enter synchronous mode if this will push the size of
    //remoteStates to 8, or if nfrags != 1
    bool enterSync = (nfrags != 1 || remoteStates.size() >= 7);

    //Transmit fragments
    for (Uint16 i=0; i < nfrags; ++i) {
      static byte pack[256];
      byte* dat = pack;
      dat += NetworkConnection::headerSize;
      io::write(dat, seq);
      io::write(dat, i);
      io::write(dat, nfrags);
      //Copy data
      unsigned datlen = (i+1==nfrags? packet.size()-i*capacity : capacity);
      memcpy(dat, &packet[i*capacity], datlen);
      dat += datlen;
      //Send and record
      if (i < MAX_SYNC_OUTPACK) {
        NetworkConnection::seq_t netseq = send(pack, dat-pack);
        pending.insert(make_pair(netseq, seq));
        if (enterSync)
          syncPending.insert(make_pair(netseq, vector<byte>(pack, dat)));
      } else {
        //Maximum transmission rate exceeded, queue for later
        syncQueue.push_back(vector<byte>(pack, dat));
      }
    }

    //Record current state
    remoteStates.insert(make_pair(seq, state));

    //Mark clean
    dirty = false;
  }
}

void OutputBlockGeraet::ack(NetworkConnection::seq_t seq) throw() {
  //Remove synchronous entry if exists
  syncPending.erase(seq);
  //Get the mutation sequence number
  block_geraet_seq mseq;
  pending_t::iterator it = pending.find(seq);
  //This must ALWAYS exist
  assert(it != pending.end());
  mseq = it->second;
  pending.erase(it);

  //If there is a queued synchronous packet, send it now
  if (!syncQueue.empty()) {
    vector<byte>& packet = syncQueue.front();
    NetworkConnection::seq_t netseq = send(&packet[0], packet.size());
    syncPending.insert(make_pair(netseq, packet));
    pending.insert(make_pair(netseq, mseq));
    syncQueue.pop_front();
  }

  //Do nothing else unless we are no longer in synchronous mode (due to the
  //above syncPending operation)
  if (syncPending.empty()) {
    //See if this is a non-obsolete state
    remoteStates_t::iterator sit = remoteStates.find(mseq);
    if (sit != remoteStates.end()) {
      //Update to remote state
      old = sit->second;
      //Remove all states before and including this one
      while (!remoteStates.empty() && remoteStates.begin()->first <= mseq)
        remoteStates.erase(remoteStates.begin());
    }
  }
}

void OutputBlockGeraet::nak(NetworkConnection::seq_t seq) throw() {
  //Get the mutation sequence number
  pending_t::iterator it = pending.find(seq);
  assert(it != pending.end());
  block_geraet_seq mseq = it->second;
  pending.erase(it);

  /* If in synchronous mode, we only care about fragments of the
   * last state; the NAK can be ignored otherwise (the old states
   * will be cleared when the last packet is confirmed received).
   * If it is one of the synchronous packet fragments, retransmit
   * it and move it to its new location in the map.
   *
   * In high-speed mode, delete the remoteState corresponding to
   * the mseq; if this leaves remoteStates empty, set dirty to
   * true to retransmit the changes on the next update.
   */
  if (syncPending.empty()) {
    remoteStates.erase(mseq);
    if (remoteStates.empty())
      dirty = true;
  } else {
    syncPending_t::iterator sit = syncPending.find(seq);
    if (sit != syncPending.end()) {
      //Retransmit and move to new location in map
      vector<byte> packet(sit->second);
      syncPending.erase(sit);
      NetworkConnection::seq_t netseq = send(&packet[0], packet.size());
      syncPending.insert(make_pair(netseq, packet));
      pending.insert(make_pair(netseq, mseq));
    }
  }
}

bool OutputBlockGeraet::isSynchronised() const throw() {
  return syncPending.empty() && remoteStates.empty();
}
