/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.11
 * @brief Implements the network testing functions
 */

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include <SDL.h>
#include <asio.hpp>

#include "antenna.hxx"
#include "tuner.hxx"
#include "network_assembly.hxx"
#include "network_connection.hxx"
#include "network_geraet.hxx"
#include "connection_listener.hxx"
#include "synchronous_control_geraet.hxx"
#include "block_geraet.hxx"
#include "src/sim/game_field.hxx"
#include "src/core/game_state.hxx"
#include "src/graphics/asgi.hxx"
#include "src/globals.hxx"

using namespace std;

#define DIM 1024

class NetworkTestBase {
public:
  virtual ~NetworkTestBase() {}
  void draw() {
    const vector<byte>& data = getData();
    for (unsigned y = 0; y < DIM; ++y) {
      asgi::begin(asgi::Points);
      for (unsigned x = 0; x < DIM; ++x) {
        float c = ((unsigned int)data[y*DIM+x])/255.0f;
        asgi::colour(1,c,c,1);
        asgi::vertex(x/(float)DIM, y*vheight/(float)DIM);
      }
      asgi::end();
    }
  }

protected:
  virtual const vector<byte>& getData() const = 0;
};

class NetworkTestListen:
public GameState,
public OutputBlockGeraet,
private NetworkTestBase {
public:
  bool randomising;
  NetworkAssembly* assembly;
  GameState* ret;

  NetworkTestListen(NetworkAssembly* na, AsyncAckGeraet* aag)
  : OutputBlockGeraet(DIM*DIM, aag, DSIntrinsic),
    randomising(false), assembly(na), ret(NULL)
  {
  }

  virtual const vector<byte>& getData() const {
    return state;
  }

  virtual GameState* update(float et) noth {
    //GameState::update
    assembly->update((unsigned)et);
    if (randomising) {
      for (unsigned i=0; i<16; ++i)
        state[rand()%state.size()] = rand();
      dirty = true;
    }

    return ret;
  }

  virtual void draw() noth {
    NetworkTestBase::draw();
  }

  virtual void keyboard(SDL_KeyboardEvent* e) {
    if (e->type == SDL_KEYDOWN) {
      switch (e->keysym.sym) {
        case SDLK_q:
          ret = this;
          break;

        case SDLK_s: {
          //Sierpinski triangle
          memset(&state[0], 0, state.size());
          signed x = rand()%DIM, y = rand()%DIM;
          for (unsigned i=0; i<DIM*DIM; ++i) {
            signed vx, vy;
            switch (rand()%3) {
              case 0: vx = 0; vy = 0; break;
              case 1: vx = DIM; vy = 0; break;
              case 2: vx = DIM/2; vy = DIM; break;
            }

            x += (vx-x)/2;
            y += (vy-y)/2;
            if (x >= 0 && x < DIM && y >= 0 && y < DIM)
              state[y*DIM+x] = 164;
          }
          dirty = true;
        } break;

        case SDLK_r: {
          randomising ^= true;
        } break;

        case SDLK_w: {
          memset(&state[0], 255, state.size());
          dirty = true;
        } break;

        case SDLK_b: {
          memset(&state[0], 0, state.size());
          dirty = true;
        } break;

        case SDLK_g: {
          for (unsigned y=0; y<DIM; ++y)
            for (unsigned x=0; x<DIM; ++x)
              state[y*DIM+x] = y/4;
          dirty = true;
        } break;

        case SDLK_i: {
          for (unsigned y=0; y<DIM/10; ++y)
            for (unsigned x=0; x<DIM; ++x)
              state[y*DIM+x] += 32;
          dirty = true;
        } break;

        default: break;
      }
    }
  }
};

static NetworkAssembly* ntr_assembly;
class NetworkTestRun:
public GameState,
public NetworkTestBase,
public InputBlockGeraet {
  NetworkAssembly* assembly;
public:
  NetworkTestRun(AsyncAckGeraet* aag, NetworkAssembly* na)
  : InputBlockGeraet(DIM*DIM, aag, DSIntrinsic),
    assembly(na)
  {
    ::state = this;
  }

  virtual GameState* update(float et) {
    assembly->update(et);
    if (assembly->getConnection(0)->getStatus() == NetworkConnection::Zombie)
      return this;
    else
      return NULL;
  }

  virtual void draw() {
    NetworkTestBase::draw();
  }

  virtual const vector<byte>& getData() const {
    return state;
  }

  static InputNetworkGeraet* create(NetworkConnection* cxn) {
    return new NetworkTestRun(cxn->aag, ntr_assembly);
  }
};

class TestListener: public ConnectionListener {
  NetworkAssembly* nasm;
public:
  TestListener(NetworkAssembly* nas, Tuner* tuner)
  : ConnectionListener(tuner), nasm(nas)
  { }

  virtual bool acceptConnection(const Antenna::endpoint& source,
                                Antenna* antenna, Tuner* tuner,
                                std::string& errmsg,
                                std::string& errl10n)
  noth {
    NetworkConnection* cxn = new NetworkConnection(nasm, source, true);
    nasm->addConnection(cxn);
    cxn->scg->openChannel(cxn->aag, cxn->aag->num);
    NetworkTestListen* that = new NetworkTestListen(nasm, cxn->aag);
    cxn->scg->openChannel(that, 999);
    state = that;
    return true;
  }
};

void networkTestListen() {
  static GameField field(1,1);
  static NetworkAssembly assembly(&field, &antenna);
  static bool init = false;
  if (!init) {
    assembly.addPacketProcessor(
      new TestListener(&assembly, assembly.getTuner()));
    init = true;
  }

  while (assembly.numConnections() == 0) {
    assembly.update(5);
    SDL_Delay(5);
  }
}

unsigned networkTestRun(const char* addrstr, unsigned portn) {
  static GameField field(1,1);
  static NetworkAssembly assembly(&field, &antenna);
  ntr_assembly = &assembly;
  static unsigned num = 0;
  if (!num)
    num =
        NetworkConnection::registerGeraetCreator(&NetworkTestRun::create, 999);

  asio::ip::udp::endpoint endpoint(asio::ip::address::from_string(addrstr),
                                   portn);
  NetworkConnection* cxn = new NetworkConnection(&assembly, endpoint, false);
  assembly.addConnection(cxn);
  cxn->scg->openChannel(cxn->aag, cxn->aag->num);

  state = NULL;
  while (!state) {
    assembly.update(5);
    SDL_Delay(5);
  }
  return 0;
}
