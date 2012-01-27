# Contains the BootManager Application and
# related functions

$ open tcl/init.d/rc boot

# The BootManager runs through a catalogue described
# in tcl/init.d/rc (see that file for more details).
# The last entry in the BootManager should point the
# current manager to a new state or new catalogue.
class BootManager {
  inherit ::gui::Application

  # Callbacks are called once per update cycle and
  # return an integer percentage of completion (>100
  # indicates completion).
  variable currentCallback
  variable currentCallbackDescription

  variable catalogue
  variable catindex

  variable timeUntilSpinnerUpdate
  variable spinner

  variable returnState

  constructor {cat} {
    Application::constructor
  } {
    set currentCallback {}
    set catalogue $cat
    set catindex 0
    set returnState 0
    set spinner /
    set mode [new BootManagerMode \
      "\a\[(white)\a__$spinner\a\] [_ A boot [$ str boot.$catalogue.\[0\]]]"]

    set timeUntilSpinnerUpdate 256
  }

  method drawThis {} {
    ::sbg::draw
  }

  method updateThis et {
    ::sbg::update $et

    set timeUntilSpinnerUpdate [expr {$timeUntilSpinnerUpdate-$et}]
    if {$timeUntilSpinnerUpdate < 0} {
      set timeUntilSpinnerUpdate 256
      # "\u2015" is UNICODE "horizontal bar"
      switch -exact -- $spinner {
        /        {set spinner "\u2015"  }
        "\u2015" {set spinner \\        }
        \\       {set spinner |         }
        |        {set spinner /         }
      }
    }

    if {[string length $currentCallback]} {
      set code [catch { namespace eval :: $currentCallback } percent]
      if {$code != 0 && $code != 2} {
        puts stderr "Error: Unexpected boot return code $code: $percent"
        return $this
      }
      if {$percent > 100} {
        set currentCallback {}
        $mode setText "\a\[(white)\a__$spinner\a\] $currentCallbackDescription (100%)"
      } else {
        $mode setText "\a\[(white)\a__$spinner\a\] $currentCallbackDescription ($percent%)"
      }
    } else {
      if {$catindex >= [$ length boot.$catalogue] && 0 == $returnState} {
        # No exit specified, panic
        puts stderr "FATAL: While booting $catalogue: end reached and no exit specified!"
        exit 1
      }

      # Source the next file
      set index $catindex
      incr catindex
      namespace eval :: source tcl/init.d/[$ str boot.$catalogue.\[$index\]].tcl

      # Update the text for the next step
      if {$catindex < [$ length boot.$catalogue] && "" == $currentCallback} {
        $mode setText "\a\[(white)\a__$spinner\a\] [_ A boot [$ str boot.$catalogue.\[$catindex\]]]..."
      }
    }

    return $returnState
  }

  method setSubState state {
    set subapp $state
  }

  # Alters the current callback to the given description/code pair
  method setCallback {descr code} {
    set currentCallbackDescription $descr
    set currentCallback $code
    $mode setText "\a\[(white)\a__$spinner\a\] $descr"
  }

  # Resets catalogues to the catalogue specified
  method setCatalogue cat {
    set catalogue $cat
    set catindex 0
    $mode setText \
      "\a\[(white)\a__$spinner\a\] [_ A boot [$ str boot.$catalogue.\[0\]]]"
  }

  # Sets the exit state to the one specified.
  # The BootManager will return this as soon as it has no
  # substate.
  method setReturn ret {
    set returnState $ret
  }

  method getCursor {} { return busy }
}

# Simple Mode for BootManager to show a single line of text
class BootManagerMode {
  inherit ::gui::Mode

  variable label

  constructor initstr {
    ::gui::Mode::constructor
  } {
    set root [new ::gui::VerticalContainer 0 centre]
    set label [new ::gui::Label $initstr]
    $root add $label
    $root setSize 0 1 $::vheight 0
    refreshAccelerators
  }

  method setText txt {
    $label setText $txt
  }
}
