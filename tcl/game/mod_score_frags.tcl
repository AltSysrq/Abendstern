# This mixin extends processing of the kill-notification unicast
# to modify the vpeer's score according to the following table:
#   kill        +4
#   assist      +2
#   secondary   +1
#   suicide     -2
#   teamkill    -8
# This class also modifies the ships' internal scores.
set TEAMKILL_PENALTY -8
class MixinScoreFrags {
  method receiveUnicast {peer msg} {
    lassign $msg type
    if {$type == "kill-notification"} {
      lassign $msg type killtype localvp remotevp
      set teamkill no
      catch {
        if {[$this dpg $localvp team] == [$this dpgp $peer $remotevp team]} {
          set teamkill yes
        }
      }
      if {$peer == 0 && $localvp == $remotevp
      &&  ($killtype == "killer" || $killtype == "secondary")} {
        set delta -2
      } elseif {[arePlayersNeutral 0 $localvp $peer $remotevp]} {
        set delta 0
      } else {
        switch -glob -- $killtype/$teamkill {
          killer/no     { set delta 4 }
          assist/no     { set delta 2 }
          secondary?/no { set delta 1 }
          killer/yes    { set delta $::TEAMKILL_PENALTY }
          default       { set delta 0 }
        }
      }
      if {$delta != 0} {
        $this dps $localvp score [expr {[$this dpg $localvp score] + $delta}]
        set ship [$this getOwnedShip $localvp]
        if {$ship != 0} {
          $ship configure -playerScore [$this dpg $localvp score]
          $ship configure -score [expr {[$ship cget -score]+$delta}]
          if {[$ship cget -controller] != 0} {
            [$ship cget -controller] notifyScore $delta
          }
        }
      }
    }

    chain $peer $msg
  }

  # Returns true if the two peer/vpeer pairs are considered
  # neutral.
  # Default returns false.
  method arePlayersNeutral {peer0 vp0 peer1 vp1} {
    return no
  }
}
