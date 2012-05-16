# Downloads the user's abendstern.rc file from the server and merges
# select sections of it with the local configuration.

if {"" != $::abnet::userid} {
  # Remove any stale remoterc file
  file delete [homeq remoterc.tmp]

  # Find its fileid
  ::abnet::lookupFilename abendstern.rc
  $state setCallback [_ A boot acctinfo] {
    if {$::abnet::busy} {
      return 0
    } else {
      # Does the file exist?
      if {$::abnet::success} {
        # Yes, download it
        ::abnet::getf $::abnet::filenames($::abnet::userid,abendstern.rc) \
            [homeq remoterc.tmp]
        $::state setCallback [_ A boot acctinfo] {
          # Wait until the download completes
          if {$::abnet::busy} {
            return 50
          } else {
            # We have it, merge relavent sections
            # We do this manually, to ensure that the input is valid
            if {[catch {
              $ open [homeq remoterc.tmp] remote
              for {set i 0} {$i < 3} {incr i} {
                $ setf conf.ship_colour.\[$i\] \
                  [$ float remote.ship_colour.\[$i\]]
              }
              foreach colour {standard warning danger special} {
                for {set i 0} {$i < 4} {incr i} {
                  $ setf conf.hud.colours.$colour.\[$i\] \
                    [$ float conf.hud.colours.$colour.\[$i\]]
                }
              }
              if {[$ exists conf.[$ str remote.control_scheme]]
              &&  [string match *_control [$ str remote.control_scheme]]} {
                $ sets conf.control_scheme [$ str remote.control_scheme]
              }
              switch -exact -- [$ str remote.camera.mode] {
                rotation -
                velocity -
                none {
                  $ sets conf.camera.mode [$ str remote.camera.mode]
                }
              }
              $ setf conf.camera.lookahead [$ float remote.camera.lookahead]

              if {[$ exists remote.default_share_ships]} {
                catch {$ remove conf.default_share_ships}
                $ addb conf default_share_ships \
                  [$ bool remote.default_share_ships]
              }
            } err]} {
              log "Unable to merge remote config: $err"
            }

            catch {$ close remote}
            file delete [homeq remoterc.tmp]
            # Done
            return 200
          }
        }
        return 50
      } else {
        # Nope, use whatever's handy
        return 200
      }
    }
  }
}
