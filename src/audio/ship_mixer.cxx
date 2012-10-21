/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implements src/audio/ship_mixer.hxx
 */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stack>
#include <vector>
#include <utility>
#include <queue>
#include <map>
#include <fstream>
#include <iostream>

#include <SDL.h>
#include <SDL_thread.h>

#include <boost/shared_array.hpp>

#include "audio.hxx"
#include "ship_mixer.hxx"
#include "src/ship/cell/cell.hxx"
#include "src/ship/cell/square_cell.hxx"
#include "src/ship/cell/circle_cell.hxx"
#include "src/ship/ship.hxx"
#include "src/globals.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

//#define DEBUG_WRITE_TO_FILE "debug.raw"
#ifdef DEBUG_WRITE_TO_FILE
#warning "All ship audio will be written to a file!"
#warning "THIS MUST NOT BE ENABLED IN A PRODUCTION BUILD!!!"
#endif

/* The inverse-speed of sound at reinforcement zero.
 * The speed at any other reinforcement is found by
 * dividing this value by (1+sqrt(reinforcement)) and
 * rounding down.
 */
#define PRD_OF_SOUND_AT_RNF0 (AUDIO_FRQ/200)

/* The energy loss per cell at reinforcement zero.
 * The loss at any other reinforcement is found by
 * dividing this value by (1+sqrt(reinforcement)).
 */
#define LOSS_AT_RNF0 0.10f

/* How much audio to remix at a time.
 * Higher values produce higher throughput, but cause
 * poorer responsiveness to mixing abortion (which will stall
 * the main thread when the ship is modified).
 */
#define MIX_SEGMENT_SZ 512

/* The maximum number of elements in the root ship mixer before
 * we begin ignoring dynamic events.
 */
#define MAX_MIXER_SZ 32

namespace audio {
  typedef boost::shared_array<Sint16> Clip;

  /* Set to false by the main thread to indicate that current calculations by
   * the worker thread should be aborted and it is to enter the waiting state.
   */
  static volatile bool abortCurrent;

  /* Set to false by the main thread to indicate that the worker thread is
   * to terminate instead of waiting on the condition, next time it has
   * nothing to do.
   */
  static volatile bool continueRunning;

  /* Set to true if the graph is ready to be used for new things. */
  static volatile bool graphReady;

  /* Used for total synchronisation between the main and worker threads. */
  static SDL_mutex* totalMutex;
  /* Used to ensure that the worker thread is waiting. */
  static SDL_mutex* waitMutex;

  /* The worker waits on this condition when it has nothing to do, locked
   * on totalMutex.
   */
  static SDL_cond* cond;

  /* The worker thread. */
  static SDL_Thread* thread;

  /* See shipMixer definition in ShipMixer section */
  static VMixer*const quietMixer(new VMixer(0x1000, 0x3000));

  /****************************************************************************
   * BEGIN SECTION: SHIP GRAPH                                                *
   ****************************************************************************/

  /* Defines the conduction of sound from a source to the bridge.
   * delay      Delay, in samples, between the sound production and observation
   * Sint16     Volume multiplier
   * l,r        Whether the left and right channels are affected
   */
  struct ConductionElt {
    unsigned delay;
    Sint16 volume;
    bool l, r;
  };

  #define MAX_ELTS 32

  /* Since we do calculations on a separate thread, we need to copy
   * the ship information into a separate structure, which we'll also
   * use to store the conduction tables.
   */
  struct Node {
    Node* neighbours[4];
    unsigned numNeighbours;
    ConductionElt elts[MAX_ELTS];
    unsigned numElts;
  }
  /* The hard cap for ship size is 4094 cells, so hardwire to that. */
  static cells[MAX_CELL_CNT];
  static unsigned numCells;

  /* Map source Cell*s to Node*s, since the outside world has no knowledge
   * of the Node structure.
   */
  static map<Cell*,Node*> cellMap;

  static float shipReinforcement;

  /* Sets the graph for the specified Cell* array up.
   * This assumes that it is already empty.
   */
  static void graphCreate(Cell*const* c, unsigned n) {
    //Start by mapping cells to nodes
    Node* curr = cells;
    for (unsigned i=0; i<n; ++i)
      if (!c[i]->isEmpty)
        cellMap[c[i]] = curr++;

    //Now copy other information
    numCells = cellMap.size();
    for (map<Cell*,Node*>::const_iterator it = cellMap.begin(); it != cellMap.end(); ++it) {
      Cell* cell = it->first;
      Node* node = it->second;
      node->numNeighbours = cell->numNeighbours();
      memset(node->neighbours, 0, sizeof(node->neighbours));
      for (unsigned i=0; i<node->numNeighbours; ++i)
        if (cell->neighbours[i] && !cell->neighbours[i]->isEmpty)
          node->neighbours[i] = cellMap[cell->neighbours[i]];
    }
  }

  /* Updates the graph to compensate for cells no longer present in the provided
   * array. Removed cells are added to the specified vector.
   * Results are undefined if new, non-empty cells are present.
   */
  static void graphUpdate(Cell*const* c, unsigned n, set<Cell*>& removed) {
    //Copy the old cellMap so we can use it for taking the difference
    map<Cell*,Node*> oldMap(cellMap);
    //Make a set so we have Cell*s in the same order as the map
    set<Cell*> cellset;
    for (unsigned i=0; i<n; ++i)
      if (!c[i]->isEmpty)
        cellset.insert(c[i]);

    for (map<Cell*,Node*>::const_iterator it = oldMap.begin(); it != oldMap.end(); ++it) {
      Cell* cell = it->first;
      Node* node = it->second;
      //Does it exist?
      if (cellset.end() == cellset.find(cell)) {
        //Nope, remove from map and nullify respective neighbours
        cellMap.erase(cellMap.find(cell));
        removed.insert(cell);
        for (unsigned i=0; i<4; ++i)
          if (node->neighbours[i])
            for (unsigned j=0; j<4; ++j)
              if (node->neighbours[i]->neighbours[j] == node)
                node->neighbours[i]->neighbours[j] = NULL;
      }
    }
  }

  /* Maps the Cell* vector into the provided empty Node* vector. */
  static void mappingCreate(const vector<Cell*>& c, vector<Node*>& n) {
    for (unsigned i=0; i<c.size(); ++i)
      n.push_back(cellMap[c[i]]);
  }

  /* Updates the given Cell* and Node* vectors, given a list of Cell*s that
   * were removed.
   */
  static void mappingUpdate(vector<Cell*>& c, vector<Node*>& n, const set<Cell*>& removed) {
    for (unsigned i=0; i<c.size(); ++i) {
      set<Cell*>::iterator it = removed.find(c[i]);
      if (it != removed.end()) {
        n.erase(n.begin() + i);
        c.erase(c.begin() + i--);
      }
    }
  }

  /* Returns the index of the first node in the second's neighbour array. */
  static inline unsigned revn(const Node* a, const Node* b) {
    for (unsigned i=0;; ++i) {
      assert(i < b->numNeighbours);
      if (b->neighbours[i] == a)
        return i;
    }
  }

  struct graphop {
    Node* node;
    unsigned from;
    unsigned delay;
    float vol;
    bool l, r;
  };
  /* Calculates the conductions for all nodes in the graph.
   * Returns early if abortCurrent is set to true.
   *
   * The following initialisation is performed:
   * + All nodes have numElts set to 0
   * + Node 0 (the bridge) is given a conduction of 0 delay and 100% volume.
   *
   * After that, a queue is created and the following algorithm performed until
   * the queue is emptied:
   * + The head is removed; it contains the Node* to work on, the neighbour index
   *   from whence it came, as well as the delay, volume values, and the channels.
   * + A ConductionElt is added to that Node* if one of these conditions is met:
   *   - numElts < MAX_ELTS: Add to end of array, increment numElts
   *   - One elt already there has a lower volume: Replace the elt that has the lowest volume
   * + If this elt was not used, go back to the beginning
   * + Otherwise, add new items to the queue for each neighbour, adding the appropriate value
   *   to the delay and multiplying the volume by the energy loss factor and the conductance
   *   defined below. Abort this step for any neighbour that results in a volume less than 0x100.
   *
   * Volume is tracked internally with floats, and starts at a volume higher than 100% (so that
   * sound can be conducted farther).
   *
   * The queue is started by adding an item for each neighbour of the bridge, which is the same as
   * the last step above, except that channels and volume is determined as below: (forward is right)
   *
   * Square/circle:      l/10.0
   *                         |
   *             lr/7.50   --+-- lr/7.50
   *                         |
   *                     r/10.0
   *
   * Equilateral triangle:   l/10.0
   *                          /
   *             lr/7.50   --<
   *                          \
   *                          r/10.0
   * Conductance is determined by the type of the cell and the neighbour arrangement. The table
   * below has the outgoing (since we do the graph in reverse) signal from the left. Each number
   * indicates the volume multiplier for incomming from that direction.
   *                                               0.50
   * Square/circle:                                  |
   *      1.0 --+          0.0 --+-- 1.0      0.50 --+
   *
   *                           0.30             0.30
   *                             |                |
   *     0.50 --+         0.40 --+          0.0 --+-- 0.40
   *            |                |                |
   *          0.50             0.30             0.30
   *
   *                                0.40
   *                                 |
   *      0.0 --+-- 0.60       0.0 --+-- 0.60
   *            |
   *           0.40
   *
   * Equilateral/right:       0.667
   *                            /
   *    1.0 --*         0.33 --*                  0.33 --*
   *                                                      \
   *          0.40                                      0.667
   *           /
   *   0.20 --*
   *          \
   *         0.40
   */
  void calcConductions() {
    graphReady = false;
    //Initialise
    for (unsigned i=0; i<MAX_CELL_CNT; ++i)
      cells[i].numElts = 0;
    cells[0].elts[0].delay = 0;
    cells[0].elts[0].volume = 0x7FFF;
    cells[0].elts[0].l = cells[0].elts[0].r = true;
    cells[0].numElts = 1;

    unsigned transitTime = (unsigned)(PRD_OF_SOUND_AT_RNF0 / (1.0f + sqrt(shipReinforcement)));
    float energyLoss = (1.0f - LOSS_AT_RNF0 / (1.0f + sqrt(shipReinforcement)));

    //Initial queue setup
    queue<graphop> ops;
    if (cells[0].numNeighbours == 4) {
      //Square/circle
      if (cells[0].neighbours[0]) {
        graphop op = { cells[0].neighbours[0], revn(&cells[0], cells[0].neighbours[0]),
                       transitTime, 1.5f, true, true };
        ops.push(op);
      }
      if (cells[0].neighbours[1]) {
        graphop op = { cells[0].neighbours[1], revn(&cells[0], cells[0].neighbours[1]),
                       transitTime, 2.0f, true, false };
        ops.push(op);
      }
      if (cells[0].neighbours[2]) {
        graphop op = { cells[0].neighbours[2], revn(&cells[0], cells[0].neighbours[2]),
                       transitTime, 1.5f, true, true };
        ops.push(op);
      }
      if (cells[0].neighbours[3]) {
        graphop op = { cells[0].neighbours[3], revn(&cells[0], cells[0].neighbours[3]),
                       transitTime, 2.0f, false, true };
        ops.push(op);
      }
    } else {
      //Triangle
      if (cells[0].neighbours[0]) {
        graphop op = { cells[0].neighbours[0], revn(&cells[0], cells[0].neighbours[0]),
                       transitTime, 10.0f, true, false };
        ops.push(op);
      }
      if (cells[0].neighbours[1]) {
        graphop op = { cells[0].neighbours[1], revn(&cells[0], cells[0].neighbours[1]),
                       transitTime, 7.5f, true, true };
        ops.push(op);
      }
      if (cells[0].neighbours[2]) {
        graphop op = { cells[0].neighbours[2], revn(&cells[0], cells[0].neighbours[2]),
                       transitTime, 10.0f, false, true };
        ops.push(op);
      }
    }

    //OK, ready to run
    while (!ops.empty() && !abortCurrent) {
      const graphop& o(ops.front());

      //Do we instert it?
      if (o.node->numElts < MAX_ELTS) {
        //Yes, addition case
        unsigned ix = o.node->numElts ++;
        o.node->elts[ix].delay = o.delay;
        o.node->elts[ix].volume = ((Sint32)(0x7FFF * min(1.0f, o.vol)));
        o.node->elts[ix].l = o.l;
        o.node->elts[ix].r = o.r;
      } else {
        //See if anything is queiter
        unsigned ix = 0;
        for (unsigned i=1; i<MAX_ELTS; ++i)
          if (o.node->elts[i].volume < o.node->elts[ix].volume)
            ix = i;

        if (o.node->elts[ix].volume < o.vol*0x7FFF) {
          //Insert, ix is the minimum
          o.node->elts[ix].delay = o.delay;
          o.node->elts[ix].volume = ((Sint32)(0x7FFF * min(1.0f, o.vol)));
          o.node->elts[ix].l = o.l;
          o.node->elts[ix].r = o.r;
        } else {
          //Can't insert, skip this one
          ops.pop();
          continue;
        }
      }

      /* Generate a "cell code" which we can use in a switch statement.
       * So it is easy to use hex, each item uses one nybble. Neighbours
       * run counterclockwise from the source node. For triangles, the
       * nybbles are
       *   <neighbour+1?><neighbour+2?><numNeighbours><currentNeighbourOff>
       * And for squares
       *   <neighbour+1?><neighbour+2?><neighbour+3><numNeighbours><currentNeighbourOff>
       */
      unsigned code = 0;
      for (unsigned i=1; i<o.node->numNeighbours; ++i) {
        code |= (o.node->neighbours[(i+o.from)%o.node->numNeighbours]? 1:0);
        code <<= 4;
      }
      code |= o.node->numNeighbours;
      code <<= 4;

      for (unsigned i=0; i<o.node->numNeighbours; ++i) {
        Node* n = o.node->neighbours[(i+o.from)%o.node->numNeighbours];
        if (n) {
          graphop op = { n, revn(o.node, n), o.delay+transitTime, o.vol*energyLoss, o.l, o.r };
          switch (code | i) {
            case 0x00040:
            case 0x00030:
              //100% reflection back
              break;
            //Special quad cases
            case 0x10041:
            case 0x00143:
              //Around right-angle
              op.vol *= 0.50f;
              break;
            case 0x10040:
            case 0x00140:
              //Reflected by right-angle
              op.vol *= 0.50f;
              break;
            case 0x01042:
              //Pass through
              break;
            case 0x01040:
            case 0x11040:
            case 0x01140:
            case 0x11140:
              //All passed through, nothing reflected
              op.vol = 0;
              break;
            case 0x10140:
              //T-intersection reflection
              op.vol *= 0.4f;
              break;
            case 0x10141:
            case 0x10143:
              //T-intersection leg
              op.vol *= 0.30f;
              break;
            case 0x11042:
            case 0x01142:
              //Side-T pass-through
              op.vol *= 0.60f;
              break;
            case 0x11041:
            case 0x01143:
              //Side-T dispersion
              op.vol *= 0.40f;
              break;
            case 0x11142:
              //4-way pass-through
              op.vol *= 0.4f;
              break;
            case 0x11141:
            case 0x11143:
              //4-way side
              op.vol *= 0.30f;
              break;

            //Special triangle cases
            case 0x1030:
            case 0x0130:
              //Corner reflection
              op.vol *= 0.3333f;
              break;
            case 0x1031:
            case 0x0132:
              //Corner pass-through
              op.vol *= 0.6667f;
              break;
            case 0x1130:
              //3-way reflection
              op.vol *= 0.2f;
              break;
            case 0x1131:
            case 0x1132:
              //3-way leg
              op.vol *= 0.4f;
              break;
            default:
              cerr << "FATAL: Unexpected code in " __FILE__ ":" << __LINE__ << ": " << code << endl;
              exit(EXIT_PROGRAM_BUG);
          }

          if (op.vol > 1.0f/256.0f)
            ops.push(op);
        }
      } //for loop on neighbours

      //Done with this one
      ops.pop();
    } //While ops

    graphReady = true;
  }
  /****************************************************************************
   * END SECTION: SHIP GRAPH                                                  *
   ****************************************************************************/

  /****************************************************************************
   * BEGIN SECTION: REMIXING                                                  *
   ****************************************************************************/
  static const Sint16 zero[1024] = {0};

  struct remix_data {
    bool wasInit;
    struct remix_data_inner {
      const Sint16* d, *end;
      Sint16 volume;
    } inputs[MAX_ELTS];
    unsigned sam;

    remix_data() : wasInit(false), sam(0) { }
  };
  /* Remixes input data according to the specified ConductionElts.
   * Before each elt's delay, the pre data are used. When pre0 is
   * exhausted, pre1 will be used and looped. After the delay, the
   * post data are used, in the same way as pre.
   * The final mandatory argument is internally-used data used to allow calls
   * on the same data to be broken across invocations.
   * If the first optional argument is true, each input will start at the offset of
   * its delay.
   *
   * This function is fully thread-safe, assuming the writable data are not
   * shared between threads.
   *
   * Returns true if at least one input is not on post1.
   */
  static bool remix(Sint16* dst, unsigned numSamples,
                    const ConductionElt* elts, unsigned numElts,
                    const Sint16* pre0,  unsigned pre0len,
                    const Sint16* pre1,  unsigned pre1len,
                    const Sint16* post0, unsigned post0len,
                    const Sint16* post1, unsigned post1len,
                    remix_data& dat,
                    bool addOffset = false,
                    float volmul = 1) {

    if (!dat.wasInit){
      for (unsigned i=0; i<numElts; ++i) {
        dat.inputs[i].d = pre0 + (addOffset? pre0len - elts[i].delay % pre0len : 0);
        dat.inputs[i].end = pre0+pre0len;
        if (dat.inputs[i].d == dat.inputs[i].end) dat.inputs[i].d = pre0; //happens if elts[i].delay % pre0len == 0
        dat.inputs[i].volume = (Sint16)max(-32768.0f, min(32767.0f, volmul * elts[i].volume));
      }
      dat.wasInit = true;
    }

    for (unsigned sam=0; sam<numSamples; ++sam) {
      signed left=0, right=0;
      for (unsigned i=0; i<numElts; ++i) {
        //Don't reset if we'll be in the same array
        if (sam+dat.sam == elts[i].delay
        &&  !(dat.inputs[i].d >= pre0 && dat.inputs[i].d < pre0+pre0len && pre0==post0)
        &&  !(dat.inputs[i].d >= pre1 && dat.inputs[i].d < pre1+pre1len && pre1==post0)) {
          dat.inputs[i].d = post0;
          dat.inputs[i].end = post0+post0len;
        }
        Sint16 d = *dat.inputs[i].d++;
        muleq(d, dat.inputs[i].volume);
        if (elts[i].l) left += d;
        if (elts[i].r) right += d;

        if (dat.inputs[i].d == dat.inputs[i].end) {
          if (sam+dat.sam >= elts[i].delay) {
            dat.inputs[i].d = post1;
            dat.inputs[i].end = post1+post1len;
          } else {
            dat.inputs[i].d = pre1;
            dat.inputs[i].end = pre1+pre1len;
          }
        }
      }

      if (left  < -0x8000) left  = -0x8000;
      if (left  >  0x7FFF) left  =  0x7FFF;
      if (right < -0x8000) right = -0x8000;
      if (right >  0x7FFF) right =  0x7FFF;
      *dst++ = left;
      *dst++ = right;
    }

    dat.sam += numSamples;

    //Return true if any input is not within post1
    for (unsigned i=0; i<numElts; ++i)
      if (!(dat.inputs[i].d >= post1 && dat.inputs[i].d < post1+post1len))
        return true;
    return false;
  }

  /* Simple AudioSource that passes through the remix function.
   * If abortCurrent becomes true, and abortable is true, its get
   * function will do nothing.
   */
  class ShipRemixer: public AudioSource {
    bool alwaysReturnTrue;

    const ConductionElt* elts;
    unsigned nelts;
    const Sint16* a, *b, *c, *d;
    unsigned al, bl, cl, dl;
    bool addoff, canAbort;
    float volmul;

    remix_data dat;

    public:
    ShipRemixer(bool _alwaysReturnTrue,
                const ConductionElt* e, unsigned ne,
                const Sint16* a_, unsigned al_,
                const Sint16* b_, unsigned bl_,
                const Sint16* c_, unsigned cl_,
                const Sint16* d_, unsigned dl_,
                bool ao = false,
                float vm = 1.0f,
                bool abortable = false)
    : alwaysReturnTrue(_alwaysReturnTrue),
      elts(e), nelts(ne),
      a(a_), b(b_), c(c_), d(d_),
      al(al_), bl(bl_), cl(cl_), dl(dl_),
      addoff(ao), canAbort(abortable), volmul(vm)
    { }

    virtual bool get(Sint16* dst, unsigned numVals) {
      //Output is stereo, so num samples is half the number of values
      return (canAbort && abortCurrent) ||
             remix(dst, numVals/2,
                   elts, nelts,
                   a, al, b, bl,
                   c, cl, d, dl,
                   dat, addoff, volmul) || alwaysReturnTrue;
    }
  };
  /****************************************************************************
   * END SECTION: REMIXING                                                    *
   ****************************************************************************/

  /****************************************************************************
   * BEGIN SECTION: AMBIENT EFFECTS                                           *
   ****************************************************************************/

  /* Simply a collection of all ambient effects that exist.
   * This will be populated at static-load time.
   * Since static initialisation order is not guaranteed
   * between files (much less properly ordered according
   * to dependency), this is an uninitialised pointer.
   * New items must be added with the ShipAmbient_impl
   * constructor, which properly initialises this if
   * necessary.
   */
  static vector<ShipAmbient_impl*>* ambients;

  /* Simple AudioSource to play an ambient effect.
   * Allows altering the volume while playing.
   * Since it depends on the contents of ShipAmbient_impl,
   * its functions are implemented below that class.
   *
   * Modifications to this class will only performed by the
   * main thread by in-game events, so the audio thread will
   * be locked (therefore no further locking is necessary).
   */
  class ShipAmbientPlayer: public AudioSource {
    friend class ShipAmbient_impl;
    ShipAmbient_impl* ambient;

    enum Stage { Init=0, Middle, Final, Terminate } stage;
    unsigned ix;
    Sint16 volume;

    public:
    ShipAmbientPlayer(ShipAmbient_impl* a, Sint16 vol)
    : ambient(a), stage(Init), ix(0), volume(vol)
    { }

    virtual bool get(Sint16* dst, unsigned cnt);

    private:
    bool getsub(Sint16* dst, unsigned cnt, const Clip& init, const Clip& mid, const Clip& fin,
                                           unsigned leninit, unsigned lenmid, unsigned lenfin);
  };

  /* Contains the actual data for ambient effects.
   */
  struct ShipAmbient_impl {
    /* The lengths of the source initial, middle, and final data */
    unsigned inlen, midlen, finlen;
    /* The source initial, middle, and final data */
    Sint16* init, * middle, * final;

    float volmul;

    /* The sound thread must lock this when reading the Clip variables,
     * and the worker when writing them.
     */
    SDL_mutex* lock;
    /* The rendered arrays, or NULL if that isn't done yet. */
    Clip cin, cmid, cfin;
    /* The lengths of the rendered arrays. */
    unsigned cinlen, cmidlen, cfinlen;

    /* The raw Cell* sources. */
    vector<Cell*> sources;
    /* Remapped to internal Node*s for the current graph. */
    vector<Node*> nodesrc;

    /* The current player, or NULL if there is no player.
     * Also set to NULL when transitioning to Final, since
     * a new one playing Init must be stacked on top in
     * this case.
     */
    ShipAmbientPlayer* player;

    ShipAmbient_impl(AudioSource* src, float vm, unsigned il, unsigned ml, unsigned fl)
    : inlen(il != 1? il : ml), midlen(ml), finlen(fl != 1? fl : ml),
      init(inlen? new Sint16[inlen] : NULL),
      middle(new Sint16[midlen]),
      final(finlen? new Sint16[finlen] : NULL),
      volmul(vm),
      lock(SDL_CreateMutex()),
      cinlen(0), cmidlen(0), cfinlen(0),
      player(NULL)
    {
      if (il > 1)
        src->get(init, inlen);
      src->get(middle, midlen);
      if (fl > 1)
        src->get(final, finlen);
      if (il == 1)
        memcpy(init, middle, midlen*2);
      if (fl == 1)
        memcpy(final, middle, midlen*2);

      delete src;

      static bool hasAmbients=false;
      if (!hasAmbients) {
        ambients = new vector<ShipAmbient_impl*>;
        hasAmbients = true;
      }
      ambients->push_back(this);
    }

    virtual ~ShipAmbient_impl() {
      delete[] init;
      delete[] middle;
      delete[] final;
      SDL_DestroyMutex(lock);
    }

    void set(Sint16 vol) {
      if (!cmid) return;

      if (vol) {
        if (sources.empty()) return;
        if (player)
          player->volume = vol;
        else {
          player = new ShipAmbientPlayer(this, vol);
          SDL_LockAudio();
          quietMixer->add(player);
          SDL_UnlockAudio();
        }
      } else {
        if (player) {
          SDL_LockAudio();
          player->stage = player->Final;
          player->ix = 0;
          SDL_UnlockAudio();
          player = NULL;
        }
      }
    }

    void addSource(Cell* src) {
      sources.push_back(src);
    }
  };

  //Implementation of ShipAmbientPlayer
  bool ShipAmbientPlayer::get(Sint16* dst, unsigned cnt) {
    SDL_mutexP(ambient->lock);
    Clip init(ambient->cin), mid(ambient->cmid), fin(ambient->cfin);
    unsigned initl(ambient->cinlen), midl(ambient->cmidlen), finl(ambient->cfinlen);
    SDL_mutexV(ambient->lock);
    return getsub(dst, cnt, init, mid, fin, initl, midl, finl);
  }

  bool ShipAmbientPlayer::getsub(Sint16* dst, unsigned cnt,
                                 const Clip& init, const Clip& mid, const Clip& fin,
                                 unsigned initl, unsigned midl, unsigned finl) {
    while (cnt) {
      Clip src;
      unsigned slen;
      /* Length will be zero when the clip is NULL, so we can
       * safely ignore the special cases.
       */
      switch (stage) {
        case Init:
          src = init;
          slen = initl;
          break;
        case Middle:
          src = mid;
          slen = midl;
          break;
        case Final:
          src = fin;
          slen = finl;
          break;
        default:
          //End of stream
          memset(dst, 0, cnt*2);
          return false;
      }
      //We need to be able to handle the length changing, possibly
      //invalidating ix
      if (ix > slen) ix = slen;
      unsigned num = (cnt <= slen-ix? cnt : slen-ix);
      if (num) memcpy(dst, &src[ix], num*2);
      for (unsigned i=0; i<num; ++i)
        muleq(dst[i], volume);

      cnt -= num;
      dst += num;
      ix += num;
      if (cnt) {
        //Exhausted this stage
        //If in Middle, loop;
        //otherwise, go to next if we still have data
        ix = 0;
        if (stage != Middle || !src || !slen)
          switch (stage) {
            case Init:
              stage = Middle; break;
            case Middle:
              stage = Final; break;
            case Final:
              stage = Terminate; break;
            default: break;
          }
      }
    }

    return true;
  }

  ShipAmbient::ShipAmbient(AudioSource* src, float vm, unsigned midl, unsigned initl, unsigned finl)
  : dat(new ShipAmbient_impl(src, vm, initl, midl, finl))
  { }

  void ShipAmbient::set(Sint16 v) {
    dat->set(v);
  }

  void ShipAmbient::addSource(Cell* c) {
    dat->addSource(c);
  }

  /* Performs all ambient remixing for a single effect.
   * This assumes a number of things:
   * + It is run by the worker thread
   * + All sources are properly populated
   * + The Cell*->Node* internal remapping is complete
   * + All calculations for the ship are complete
   *
   * The length of each segment is the length of the source segement
   * plus the longest delay of any input, except for the middle, which
   * is simply the input length. The remixing pattern
   * for the three cases is:
   *   init:    (silence->silence)->(init->middle)      no offset
   *   middle:  (middle->middle)->(middle->middle)      offset added
   *   final:   (middle->middle)->(final->silence)      offset added
   */
  static void ambientRemixing(ShipAmbient_impl* ambient) {
    Clip init, mid, fin;
    unsigned initl, midl, finl;

    unsigned baselen = 0;
    //Determine base length
    for (unsigned i=0; i<ambient->nodesrc.size(); ++i) {
      Node* n = ambient->nodesrc[i];
      for (unsigned j=0; j<n->numElts; ++j)
        if (n->elts[j].delay > baselen)
          baselen = n->elts[j].delay;
    }

    //Handle init
    if (ambient->init) {
      initl = baselen + ambient->inlen;

      //Allocate (times two since output is stereo)
      init = Clip(new Sint16[initl*2]);
      //Populate a mixer with remixers for each source
      Mixer mixer;
      for (unsigned i=0; i<ambient->nodesrc.size(); ++i)
        mixer.add(new ShipRemixer(true,
                                  ambient->nodesrc[i]->elts, ambient->nodesrc[i]->numElts,
                                  zero, lenof(zero), zero, lenof(zero),
                                  ambient->init, ambient->inlen, ambient->middle, ambient->midlen,
                                  false, ambient->volmul, true));
      //Read it all in
      for (unsigned i=0; i<initl*2 && !abortCurrent; i += MIX_SEGMENT_SZ)
        mixer.get(&init[i], initl*2 - i > MIX_SEGMENT_SZ? MIX_SEGMENT_SZ : initl*2-i);
    } else initl = 0;

    if (abortCurrent) return;

    //Mid
    //Start a new scope so the temporary mixer we create will go out of scope
    {
      midl = ambient->midlen; //Since this is looped, it is simply the input length

      //Allocate
      mid = Clip(new Sint16[midl*2]);
      //Populate the mixer
      Mixer mixer;
      for (unsigned i=0; i<ambient->nodesrc.size(); ++i)
        mixer.add(new ShipRemixer(true,
                                  ambient->nodesrc[i]->elts, ambient->nodesrc[i]->numElts,
                                  ambient->middle, ambient->midlen, ambient->middle, ambient->midlen,
                                  ambient->middle, ambient->midlen, ambient->middle, ambient->midlen,
                                  true, ambient->volmul, true));
      //Read in
      for (unsigned i=0; i<midl*2 && !abortCurrent; i += MIX_SEGMENT_SZ)
        mixer.get(&mid[i], midl*2 - i > MIX_SEGMENT_SZ? MIX_SEGMENT_SZ : midl*2-i);
    }

    if (abortCurrent) return;

    //And the same for final
    if (ambient->final) {
      finl = baselen + ambient->finlen;
      fin = Clip(new Sint16[finl*2]);

      Mixer mixer;
      for (unsigned i=0; i<ambient->nodesrc.size(); ++i)
        mixer.add(new ShipRemixer(false,
                                  ambient->nodesrc[i]->elts, ambient->nodesrc[i]->numElts,
                                  ambient->middle, ambient->midlen, ambient->middle, ambient->midlen,
                                  ambient->final, ambient->finlen, zero, lenof(zero),
                                  true, ambient->volmul, true));
      //Read in
      for (unsigned i=0; i<finl*2 && !abortCurrent; i += MIX_SEGMENT_SZ)
        mixer.get(&fin[i], finl*2 - i > MIX_SEGMENT_SZ? MIX_SEGMENT_SZ : finl*2-i);
    } else finl = 0;

    if (abortCurrent) return; //Don't save, data is not valid

    //Done, write back
    SDL_mutexP(ambient->lock);
    ambient->cin = init;
    ambient->cinlen = initl*2;
    ambient->cmid = mid;
    ambient->cmidlen = midl*2;
    ambient->cfin = fin;
    ambient->cfinlen = finl*2;
    SDL_mutexV(ambient->lock);
  }

  /***************************************************************************
   * END SECTION: AMBIENT EFFECTS                                            *
   ***************************************************************************/

  /***************************************************************************
   * BEGIN SECTION: STATIC EFFECTS                                           *
   ***************************************************************************/

  /* Collection of all static effects that exist.
   * This will be populated at static-load time.
   * Only the constructor of ShipStaticEvent_impl may modify this.
   */
  static vector<ShipStaticEvent_impl*>* statics;

  /* Simple AudioSource to play a static event.
   * It captures the input array when started, removing the need for
   * any synchronisation later.
   */
  class ShipStaticEventPlayer: public AudioSource {
    Clip clip;
    unsigned ix, len;
    Sint16 volume;

    public:
    ShipStaticEventPlayer(ShipStaticEvent_impl*, Sint16);
    virtual bool get(Sint16*, unsigned);
  };

  struct ShipStaticEvent_impl {
    Clip clip;
    unsigned len;

    Sint16* dat;
    unsigned datlen;

    SDL_mutex* lock;

    vector<Cell*> sources;
    vector<Node*> nodesrc;

    float volmul;

    ShipStaticEvent_impl(AudioSource* src, float vm, unsigned length)
    : len(0), dat(new Sint16[length]), datlen(length), lock(SDL_CreateMutex()), volmul(vm)
    {
      src->get(dat, datlen);
      delete src;

      static bool hasStatics = false;
      if (!hasStatics) {
        hasStatics = true;
        statics = new vector<ShipStaticEvent_impl*>;
      }
      statics->push_back(this);
    }

    virtual ~ShipStaticEvent_impl() {
      delete[] dat;
    }

    void play(Sint16 vol) {
      if (!clip) return;

      //We must lock the audio thread since we'll be modifying the
      //mixer, and our lock since we'll be copying clip information
      //in the constructor
      SDL_LockAudio();
      SDL_mutexP(lock);
      quietMixer->add(new ShipStaticEventPlayer(this, vol));
      SDL_mutexV(lock);
      SDL_UnlockAudio();
    }

    void addSource(Cell* c) {
      sources.push_back(c);
    }
  };

  ShipStaticEventPlayer::ShipStaticEventPlayer(ShipStaticEvent_impl* that, Sint16 v)
  : clip(that->clip), ix(0), len(that->len), volume(v)
  { }

  bool ShipStaticEventPlayer::get(Sint16* dst, unsigned cnt) {
    unsigned l = (cnt > (len - ix)? len-ix : cnt);
    memcpy(dst, &clip[ix], l*2);
    ix += l;
    for (unsigned i=0; i<l; ++i)
      muleq(dst[i], volume);

    if (l != cnt) {
      memset(dst+l, 0, (cnt-l)*2);
    }
    return ix < len;
  }

  ShipStaticEvent::ShipStaticEvent(AudioSource* src, float vm, unsigned len)
  : dat(new ShipStaticEvent_impl(src, vm, len))
  { }

  void ShipStaticEvent::play(Sint16 vol) {
    dat->play(vol);
  }

  void ShipStaticEvent::addSource(Cell* c) {
    dat->addSource(c);
  }

  /* Renders a static event's clip. This makes the same assumptions
   * as ambientRemixing.
   */
  static void staticRemixing(ShipStaticEvent_impl* evt) {
    //The length of the result is the length of the input plus the
    //longest delay
    unsigned len = 0;
    for (unsigned i=0; i<evt->nodesrc.size(); ++i)
      for (unsigned j=0; j<evt->nodesrc[i]->numElts; ++j)
        if (evt->nodesrc[i]->elts[j].delay > len)
          len = evt->nodesrc[i]->elts[j].delay;
    len += evt->datlen;

    //Similar procedure as for ambient remixing.
    //The procession for data is
    //  (zero->zero)->(input->zero)     no offset
    Mixer mixer;
    for (unsigned i=0; i<evt->nodesrc.size(); ++i)
      mixer.add(new ShipRemixer(true,
                                evt->nodesrc[i]->elts, evt->nodesrc[i]->numElts,
                                zero, lenof(zero), zero, lenof(zero),
                                evt->dat, evt->datlen, zero, lenof(zero),
                                false, evt->volmul, true));
    Clip clip(new Sint16[len*2]);
    for (unsigned i=0; i<len*2 && !abortCurrent; i+=MIX_SEGMENT_SZ)
      mixer.get(&clip[i], len*2 - i > MIX_SEGMENT_SZ? MIX_SEGMENT_SZ : len*2-i);

    if (abortCurrent) return; //Data is not valid

    //OK, save
    SDL_mutexP(evt->lock);
    evt->clip = clip;
    evt->len = len*2;
    SDL_mutexV(evt->lock);
  }

  /***************************************************************************
   * END SECTION: STATIC EFFECTS                                             *
   ***************************************************************************/

  /***************************************************************************
   * BEGIN SECTION: DYNAMIC EFFECTS                                          *
   ***************************************************************************/
  class ShipDynamicEffectPlayer: public AudioSource {
    remix_data data;
    Sint16* src;
    unsigned ix, len, origlen;

    //We need to make a copy, since we operate on this asynchronously
    //from work done on the original
    ConductionElt elts[MAX_ELTS];
    unsigned numElts;

    float volmul;

    public:
    ShipDynamicEffectPlayer(AudioSource* source, unsigned _origlen, Cell* c, float vm)
    : src(new Sint16[_origlen]), ix(0), origlen(_origlen), volmul(vm)
    {
      Node* n = cellMap[c];
      numElts = n->numElts;
      memcpy(elts, n->elts, sizeof(elts));

      //The length is the original length plus the longest delay
      len = 0;
      for (unsigned i=0; i<numElts; ++i)
        if (elts[i].delay > len)
          len = elts[i].delay;
      len += origlen;

      source->get(src, origlen);
    }

    virtual bool get(Sint16* dst, unsigned cnt) {
      remix(dst, cnt/2, elts, numElts,
            zero, lenof(zero), zero, lenof(zero),
            src, origlen, zero, lenof(zero), data,
            false, volmul);
      ix += cnt/2;
      return ix < len;
    }

    virtual ~ShipDynamicEffectPlayer() {
      delete[] src;
    }
  };

  void shipDynamicEvent(AudioSource* src, unsigned len, Cell* c, float vm) {
    if (graphReady && shipMixer->size() < MAX_MIXER_SZ) {
      SDL_LockAudio();
      SDL_mutexP(totalMutex);
      shipMixer->add(new ShipDynamicEffectPlayer(src, len, c, vm));
      SDL_mutexV(totalMutex);
      SDL_UnlockAudio();
    }
    delete src;
  }
  /***************************************************************************
   * END SECTION: DYNAMIC EFFECTS                                            *
   ***************************************************************************/

  /***************************************************************************
   * BEGIN SECTION: SHIP MIXER AND THREADING                                 *
   ***************************************************************************/
  static int workerThread(void*) throw() {
    SDL_mutexP(waitMutex);
    while (continueRunning) {
      //Begin by waiting for the signal to actually do something
      SDL_CondWait(cond, waitMutex);

      if (!continueRunning) {
        SDL_mutexV(waitMutex);
        return 0;
      }

      //OK, assume the graph is populated and begin working with it
      SDL_mutexP(totalMutex);
      calcConductions();
      SDL_mutexV(totalMutex);

      if (abortCurrent) {
        abortCurrent = false;
        continue;
      }

      //Now render all the effects
      for (unsigned i=0; i<ambients->size() && !abortCurrent; ++i)
        ambientRemixing(ambients->operator[](i));
      for (unsigned i=0; i<statics->size() && !abortCurrent; ++i)
        staticRemixing(statics->operator[](i));

      abortCurrent = false;
    }

    SDL_mutexV(waitMutex);

    return 0;
  }

  bool ShipMixer::get(Sint16* dst, unsigned cnt) {
    SDL_mutexP(totalMutex);
    Mixer::get(dst, cnt);
    SDL_mutexV(totalMutex);
    #ifdef DEBUG_WRITE_TO_FILE
    static ofstream out(DEBUG_WRITE_TO_FILE, ios::binary | ios::out);
    out.write(reinterpret_cast<char*>(dst), cnt*2);
    #endif
    return true;
  }

  void ShipMixer::init() {
    static bool hasGlobals = false;
    continueRunning = true;
    abortCurrent = false;
    if (!hasGlobals) {
      hasGlobals = true;
      cond = SDL_CreateCond();
      totalMutex = SDL_CreateMutex();
      waitMutex = SDL_CreateMutex();
      trueRoot.add(shipMixer);
      shipMixer->add(quietMixer);
    }

    thread = SDL_CreateThread(workerThread, NULL);
  }

  void ShipMixer::end() {
    reset();
    abortCurrent = true;
    SDL_mutexP(totalMutex);
    cellMap.clear();
    SDL_mutexV(totalMutex);
    SDL_mutexP(waitMutex);
    continueRunning = false;
    SDL_mutexV(waitMutex);
    SDL_CondBroadcast(cond);
    SDL_WaitThread(thread,NULL);

    SDL_LockAudio();
    for (unsigned i=0; i<ambients->size(); ++i)
      (*ambients)[i]->set(0);
    SDL_UnlockAudio();
  }

  void ShipMixer::reset() {
    abortCurrent = true;
    SDL_LockAudio();
    SDL_mutexP(totalMutex);
    cellMap.clear();
    for (unsigned i=0; i<ambients->size(); ++i) {
      ShipAmbient_impl* a = (*ambients)[i];
      SDL_mutexP(a->lock);
      a->set(0);
      a->cin = a->cmid = a->cfin = Clip(NULL);
      a->cinlen = a->cmidlen = a->cfinlen = 0;
      a->sources.clear();
      a->nodesrc.clear();
      SDL_mutexV(a->lock);
    }
    for (unsigned i=0; i<statics->size(); ++i) {
      ShipStaticEvent_impl* a = (*statics)[i];
      SDL_mutexP(a->lock);
      a->clip = Clip(NULL);
      a->len = 0;
      a->sources.clear();
      a->nodesrc.clear();
      SDL_mutexV(a->lock);
    }

    SDL_mutexV(totalMutex);
    SDL_UnlockAudio();
  }

  void ShipMixer::setShip(Cell*const* cells, unsigned ncell, float reinforcement) {
    abortCurrent = true;
    SDL_LockAudio();
    SDL_mutexP(totalMutex);
    shipReinforcement = reinforcement;
    if (cellMap.empty()) {
      //Create new maps
      graphCreate(cells, ncell);
      for (unsigned i=0; i<ambients->size(); ++i)
        mappingCreate((*ambients)[i]->sources, (*ambients)[i]->nodesrc);
      for (unsigned i=0; i<statics->size(); ++i)
        mappingCreate((*statics)[i]->sources, (*statics)[i]->nodesrc);
    } else {
      //Update old maps
      set<Cell*> removed;
      graphUpdate(cells, ncell, removed);
      for (unsigned i=0; i<ambients->size(); ++i)
        mappingUpdate((*ambients)[i]->sources, (*ambients)[i]->nodesrc, removed);
      for (unsigned i=0; i<statics->size(); ++i)
        mappingUpdate((*statics)[i]->sources, (*statics)[i]->nodesrc, removed);
    }

    SDL_mutexV(totalMutex);
    SDL_UnlockAudio();

    //Notify worker
    SDL_mutexP(waitMutex);
    SDL_mutexV(waitMutex);
    graphReady = false;
    abortCurrent = false;
    SDL_CondBroadcast(cond);
  }

  /* In order to allow dynamic events to be significantly louder,
   * we use two separate mixers. The root shipMixer performs no
   */
  ShipMixer*SHIP_MIXER_CONST shipMixer(new ShipMixer);
  /***************************************************************************
   * END SECTION: SHIP MIXER AND THREADING                                   *
   ***************************************************************************/
}
