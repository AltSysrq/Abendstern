# This mixin extends the kill-notification overseer message to
# increment the teams' scores when their members get enemy kills.
class MixinScoreTeamfrag {
  method receiveOverseer {peer msg} {
    lassign $msg type
    if {$type == "kill-notification"} {
      lassign $msg type vpeer killer
      if {$killer != {}} {
        lassign [$this internalise-pvp $killer] kp kvp
        if {$kp eq {}} return

        set ok yes
        if {$kp == $peer && $kvp == $vpeer} {
          # Suicide
          set ok no
        }
        catch {
          if {[$this dpgp $peer $vpeer team] == [$this dpgp $kp $kvp team]} {
            # Teamkill
            set ok no
          }
        }

        if {$ok} {
          set team [$this dpgp $kp $kvp team]
          set teamScores [$this dsg teamScores]
          lset teamScores $team [expr {[lindex $teamScores $team]+1}]
          $this dss teamScores $teamScores
        }
      }
    }

    chain $peer $msg
  }
}
