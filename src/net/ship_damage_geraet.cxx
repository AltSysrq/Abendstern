/**
 * @file
 * @author Jason Lingle
 * @date 2012.03.09
 * @brief Implementation of the Ship Damage Gerät.
 */

#include <map>
#include <cassert>
#include <iostream>

#include "ship_damage_geraet.hxx"
#include "network_connection.hxx"
#include "io.hxx"

#include "src/ship/ship.hxx"
#include "src/sim/blast.hxx"

#include "src/globals.hxx"

using namespace std;

#define PACKET_SIZE (2+8*4+1)

const NetworkConnection::geraet_num ShipDamageGeraet::num =
    NetworkConnection::registerGeraetCreator(&create);

ShipDamageGeraet::ShipDamageGeraet(AsyncAckGeraet* aag)
: ReliableSender(aag, OutputNetworkGeraet::DSIntrinsic),
  AAGReceiver(aag, InputNetworkGeraet::DSIntrinsic)
{ }

ShipDamageGeraet::~ShipDamageGeraet() {
  //Must remove reference to self in all remote ships
  for (map<const Ship*,NetworkConnection::channel>::const_iterator it =
          remoteShips.begin();
       it != remoteShips.end(); ++it)
    const_cast<Ship*>(it->first)->shipDamageGeraet = NULL;
}

InputNetworkGeraet* ShipDamageGeraet::create(NetworkConnection* cxn) throw() {
  return cxn->sdg;
}

void ShipDamageGeraet::addRemoteShip(const Ship* ship,
                                     NetworkConnection::channel chan)
throw() {
  remoteShips[ship] = chan;
}

void ShipDamageGeraet::delRemoteShip(const Ship* ship) throw() {
  remoteShips.erase(ship);
}

void ShipDamageGeraet::addLocalShip(NetworkConnection::channel chan,
                                    Ship* ship)
throw() {
  localShips[chan] = ship;
}

void ShipDamageGeraet::delLocalShip(NetworkConnection::channel chan) throw() {
  localShips.erase(chan);
}

void ShipDamageGeraet::shipBlastCollision(const Ship* ship, const Blast* blast)
throw() {
  //Ignore if not registered
  if (!remoteShips.count(ship)) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring blast against unknown ship " << ship << endl;
    #endif
    return;
  }

  byte packet[PACKET_SIZE + NetworkConnection::headerSize];
  byte* pack = packet+NetworkConnection::headerSize;

  io::write(pack, remoteShips[ship]);
  io::write(pack, ship->getX());
  io::write(pack, ship->getY());
  io::write(pack, ship->getRotation());
  io::write(pack, blast->getX());
  io::write(pack, blast->getY());
  io::write(pack, blast->getFalloff());
  io::write(pack, blast->getStrength());
  io::write(pack, blast->getSize());
  io::write(pack, (byte)(blast->blame));
  assert(pack == packet+sizeof(packet));
  send(packet, sizeof(packet));
}

void ShipDamageGeraet::receiveAccepted(NetworkConnection::seq_t,
                                       const byte* data, unsigned len)
throw() {
  if (len != PACKET_SIZE) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring packet of invalid size " << len
         << " to ShipDamageGeraet (expected " << PACKET_SIZE << ")" << endl;
    #endif
    return;
  }

  Uint16 chan;
  float sx, sy, bx, by, falloff, strength, size, theta;
  byte blame;
  io::read(data, chan);
  io::read(data, sx);
  io::read(data, sy);
  io::read(data, theta);
  io::read(data, bx);
  io::read(data, by);
  io::read(data, falloff);
  io::read(data, strength);
  io::read(data, size);
  io::read(data, blame);
  if (theta < 0) theta += 2*pi;
  #define VER(var) if (var != var || var < 0 || var > 1.0e3f) return
  VER(sx);
  VER(sy);
  VER(theta);
  VER(bx);
  VER(by);
  VER(falloff);
  VER(strength);
  VER(size);
  #undef VER

  if (!localShips.count(chan)) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring remote damage to unknown ship channel "
         << chan << endl;
    #endif
    return;
  }

  Ship* ship = localShips[chan];
  float* tempdat = ship->temporaryZero();
  ship->teleport(sx, sy, theta);
  //TODO: translate blame
  Blast blast(ship->getField(), blame, bx, by,
              falloff, strength,
              true, size, false, ship->isDecorative(), true);
  if (!ship->collideWith(&blast)) {
    //Remove from field and free
    ship->getField()->remove(ship);
    ship->del();
    //Deregister ahead of time (since the ship might have exactly zero
    //cells, and we could get more Blasts)
    for (map<NetworkConnection::channel,Ship*>::iterator it =localShips.begin();
         it != localShips.end(); ++it) {
      if (it->second == ship) {
        localShips.erase(it);
        break;
      }
    }
  } else {
    ship->restoreFromZero(tempdat);
    assert(ship->cells.size());
  }
}
