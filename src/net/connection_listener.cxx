/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.01
 * @brief Implementation of ConnectionListener class
 */

#include <cstring>
#include <algorithm>
#include <string>

#include "connection_listener.hxx"
#include "antenna.hxx"
#include "tuner.hxx"

using namespace std;

static const byte applicationName[] = "Abendstern";
//Just assume the hash is zero for now
static const byte protocolHash[16] = {0};

static const byte header[] = {
  0, 0, //seq
  0, 0, //chan
  2, //STX
};

ConnectionListener::ConnectionListener(Tuner* tuner) {
  tuner->trigger(header, sizeof(header), this);
}

void ConnectionListener::process(const Antenna::endpoint& source,
                                 Antenna* antenna,
                                 Tuner* tuner,
                                 const byte* data,
                                 unsigned len)
noth {
  static const byte reply_wrongApplication[] =
    "\0\0" //Seq
    "\0\0" //Chan
    "\4" //EOT
    "Wrong application: Abendstern\0" //Error message
    "wrong_application" //l10n entry, implict \0 at end
    ;
  static const byte reply_protocolMismatch[] =
    "\0\0" //Seq
    "\0\0" //Chan
    "\4" //EOT
    "Protocol mismatch\0"
    "protocol_mismatch"
    ;

  //Packet length must be exactly:
  //  header + hash + application
  //long, and the application names must match exactly
  if (len != sizeof(header)+sizeof(protocolHash)+sizeof(applicationName)
  ||  0 != memcmp(data+sizeof(header)+sizeof(protocolHash),
                  applicationName, sizeof(applicationName))) {
    antenna->send(source, reply_wrongApplication,
                  sizeof(reply_wrongApplication));
    return;
  }

  //Verify compatible protocols
  if (memcmp(data+sizeof(header), protocolHash, sizeof(protocolHash))) {
    antenna->send(source, reply_protocolMismatch,
                  sizeof(reply_protocolMismatch));
    return;
  }

  //OK
  string errmsg("Connection refused"), errl10n("connection_refused");
  if (!acceptConnection(source, antenna, tuner, errmsg, errl10n)) {
    //Refusing connection
    //Header is 5 bytes, and there is one extra byte for each term nul.
    vector<byte> reply(7+errmsg.size()+errl10n.size(), 0);
    reply[4] = 4; //EOT
    copy(errmsg.begin(), errmsg.end(), reply.begin()+5);
    copy(errl10n.begin(), errl10n.end(), reply.begin()+6+errmsg.size());
    antenna->send(source, &reply[0], reply.size());
  }
}
