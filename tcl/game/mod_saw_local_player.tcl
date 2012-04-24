# Provides a status area widget which shows the local player's score
class MixinSAWLocalPlayer {
  variable line

  # Creates the widget on the given line (default 1)
  constructor {{l 1}} {
    set line $l
  }

  method getStatusAreaElements {} {
    set l [chain]
    catch {
      lappend l $line \
          "[$this getStatsFormat 0 0]: [$this dpg 0 score]"
    }
    return $l
  }
}
