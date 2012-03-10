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

#define PACKET_SIZE (3*2+1+5*2+1)

const NetworkConnection::geraet_num ShipDamageGeraet::num =
    NetworkConnection::registerGeraetCreator(&create);

ShipDamageGeraet::ShipDamageGeraet(AsyncAckGeraet* aag)
: ReliableSender(aag, OutputNetworkGeraet::DSIntrinsic),
  AAGReceiver(aag, InputNetworkGeraet::DSIntrinsic)
{ }

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
  if (!remoteShips.count(ship)) return;

  byte packet[PACKET_SIZE + NetworkConnection::headerSize];
  byte* pack = packet+NetworkConnection::headerSize;

  io::write(pack, remoteShips[ship]);
  io::write(pack, (Uint16)(512.0f*ship->getX()));
  io::write(pack, (Uint16)(512.0f*ship->getY()));
  io::write(pack, (byte)(255.0f*ship->getRotation()/2.0f/pi));
  io::write(pack, (Uint16)(512.0f*blast->getX()));
  io::write(pack, (Uint16)(512.0f*blast->getY()));
  io::write(pack, (Uint16)(65536.0f*blast->getFalloff()));
  io::write(pack, (Uint16)(128.0f*blast->getStrength()));
  io::write(pack, (Uint16)(65536.0f*blast->getSize()));
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

  Uint16 chan, sx, sy, bx, by, falloff, strength, size;
  byte theta, blame;
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

  if (!localShips.count(chan)) return;

  Ship* ship = localShips[chan];
  float* tempdat = ship->temporaryZero();
  ship->teleport(sx/512.0f, sy/512.0f, theta/255.0f*pi*2.0f);
  //TODO: translate blame
  Blast blast(ship->getField(), blame, bx/512.0f, by/512.0f,
              falloff/65536.0f,
              true, size/65536.0f, false, ship->isDecorative(), true);
  ship->collideWith(&blast);
  ship->restoreFromZero(tempdat);
}
