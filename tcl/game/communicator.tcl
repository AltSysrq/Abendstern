# The Communicator class abstracts storage of the datp and dats dictionaries,
# communicating and validating changes to them, message passing, and peer
# management.
class Communicator {
  # The BasicGame descendent we are connected to
  protected variable game
  # The globally-shared data dictionary
  protected variable dats
  # The peer-peer data dictionary
  protected variable datp

  constructor {game_ {dats_ {}} {datp_ {}}} {
    set game $game_
    set dats $dats_
    set datp $datp_
  }

  # Returns the given datp entry; the key includes the Peer as the first path
  # element.
  method get-datp {key} {
    dict get $datp {*}$key
  }

  # Returns the dats entry at the given path.
  method get-dats {key} {
    dict get $dats {*}$key
  }

  # Alters the datp entry at the given path for the local peer
  method set-datp {key val}

  # Alters the dats entry at the given path.
  method set-dats {key val}

  # Returns a list of Peers, including the local peer.
  method get-peers {}

  # Returns whether the local peer is overseer
  method is-overseer {}

  # Returns the identity of the current overseer
  method get-overseer {}

  # Sends the given broadcast message to all peers.
  method send-broadcast {msg}

  # Sends the given unicast message to the given peer.
  method send-unicast {peer msg}

  # Sends the given to-overseer message to the overseer
  method send-overseer {msg}

  # Returns the permament numeric identifier of the given peer.
  # (This number is locally-generated, independent of the peer's NID, and will
  # be exactly zero for the local peer.)
  method get-peer-number {peer}

  # Returns whether the Communicator should be deleted when the BasicGame is.
  method delete-with-parent {}
}
