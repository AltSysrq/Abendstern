# The ShipEditorMain Mode displays several things to the user:
# + A "menu bar" at the top, with a variety of buttons. All these
#   cause the current mode to be set to none and the content of
#   the left pane to be refreshed
# + A status bar, which always shows the current status message
# + A left pane, which shows details of the current tool
# + An invisible centre component, which forwards all mouse events
#   within it to the manipulator

class ManipulatorForwarder {
  inherit ::gui::AWidget

  variable editor
  variable lastX
  variable lastY

  constructor edit {
    ::gui::AWidget::constructor
  } {
    set editor $edit
    set lastX 0
    set lastY 0
  }

  method button {spec x y} {
    switch -glob $spec:[contains $x $y] {
      DOWN:mb_left:1  {$editor manip primaryDown $x $y}
      UP:mb_left:?    {$editor manip primaryUp $x $y}
      DOWN:mb_wup:1   {$editor manip scrollUp $x $y}
      DOWN:mb_wdown:1 {$editor manip scrollDown $x $y}
      DOWN:*:1        {$editor manip secondaryDown $x $y}
      UP:*:?          {$editor manip secondaryUp $x $y}
    }
  }

  method motion {x y states} {
    if {![contains $x $y]} return
    $editor manip motion $x $y [expr {$x-$lastX}] [expr {$y-$lastY}]
    set lastX $x
    set lastY $y
  }
}

class ShipEditorMain {
  inherit ::gui::Mode

  variable editor
  variable statusBar
  variable toCallSetbar

  # Some tools require the C++-side to indicate completion
  # by resetting the mode to "none". Every frame that current_mode
  # is "none", setbar is called with this, then this is set
  # to empty
  variable proceedOnNone

  constructor {edit} {
    ::gui::Mode::constructor
  } {
    set editor $edit
    set root [new ::gui::BorderContainer]
    set menuBar [new ::gui::HorizontalContainer]
    $menuBar add [new ::gui::Button [_ A editor menu_file] "$this setbar file"]
    $menuBar add [new ::gui::Button [_ A editor menu_edit] "$this setbar edit"]
    $menuBar add [new ::gui::Button [_ A editor menu_view] "$this setbar view"]
    $menuBar add [new ::gui::Button [_ A editor menu_prop] "$this setbar prop"]
    $menuBar add [new ::gui::Button [_ A editor menu_info] "$this setbar info"]
    $menuBar add [new ::gui::Button [_ A gui         quit] "$this quit"]
    $root setElt top [new ::gui::Frame $menuBar]

    set statusBar [new ::gui::Label [_ A editor idle] left]
    $root setElt bottom [new ::gui::Frame $statusBar]

    $root setElt left [new ::gui::Frame [new ::gui::Label X]]
    $root setElt centre [new ManipulatorForwarder $editor]

    refreshAccelerators
    $root setSize 0 1 $::vheight 0

    set toCallSetbar file
    set proceedOnNone {}

    $editor manip addToHistory ;#Add the initial state
  }

  method draw {} {
    if {[string length $proceedOnNone] && "none" == [$ str edit.current_mode]} {
      setbar $proceedOnNone
      set proceedOnNone {}
    }

    if {[string length $toCallSetbar]} {
      setbar_impl $toCallSetbar
    }

    $statusBar setText [$ str edit.status_message]
    ::gui::Mode::draw
  }

  method quit {} {
    if {[$ bool edit.modified]} {
      setbar quit
    } else {
      quitReally
    }
  }

  method quitReally {} {
    $editor manip deleteShip
    $editor manip resetView
    set mount [$ str edit.mountname]
    $ revert $mount

    # Delete implicitly if only the bridge remains
    # This gets read of started-but-not-saved ships
    if {1 >= [$ length $mount.cells]} {
      set cls [string tolower [$ str $mount.info.class]]
      set name [shipMount2Name $mount]
      $ close $mount
      file delete [shipMount2Path $mount]
      for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
        if {[$ str hangar.all_ships.\[$i\]] == $name} {
          $ remix hangar.all_ships $i
        }
      }
      for {set i 0} {$i < [$ length hangar.$cls]} {incr i} {
        if {[$ str hangar.$cls.\[$i\]] == $name} {
          $ remix hangar.$cls $i
        }
      }
    }
    $ sync hangar
    $editor setMode [new ShipEditorInitMode $editor]
  }

  method setbar src {
    set toCallSetbar $src
  }

  # Setbar takes a filename fragment, converts it to its true filename,
  # and uses that Tcl script to create a $panel variable that is used
  # to replace the current left bar, after placing it into a Frame.
  # The script is sourced within the function, and therefore has full
  # access to all members, including $this.
  method setbar_impl src {
    source tcl/editor/bar$src.tcl
    $root setElt left [new ::gui::Frame $panel]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
    set toCallSetbar ""
  }

  method keyboard keyspec {
    if {$keyspec == "DOWN:--C-:k_z"} {
      $editor manip deactivateMode
      $editor manip popUndo
      $editor manip reloadShip
      $editor manip activateMode
      $ setb edit.modified yes
    } else {
      Mode::keyboard $keyspec
    }
  }

  method save {} {
    # Automatically fork if we don't own
    if {("" == $::abnet::userid && -1 != [$ int [$ str edit.mountname].info.ownerid])
    ||  ("" != $::abnet::userid &&
         $::abnet::userid != [$ int [$ str edit.mountname].info.ownerid])} {
      saveFork
      return
    }

    $ modify [$ str edit.mountname]
    $ setb [$ str edit.mountname].info.needs_uploading yes
    $ sync [$ str edit.mountname]
    $ setb edit.modified no
  }

  method saveFork {} {
    if {"" == $::abnet::userid} {
      set prefix local/
    } else {
      set prefix $::abnet::userid/
    }
    set basename [generateNewShipBasename [$ str [$ str edit.mountname].info.name]]
    $ create [homeq hangar/$basename.ship] ship:$basename
    $editor manip copyMounts ship:$basename [$ str edit.mountname]
    $ revert [$ str edit.mountname]
    $ sets edit.mountname ship:$basename
    $ appends hangar.all_ships $basename
    $ appends hangar.[string tolower [$ str edit.ship_class]] $basename
    $ modify ship:$basename
    catch { $ remove ship:$basename.info.fileid }
    $ addi ship:$basename.info fileid 0
    if {"" == $::abnet::userid} {
      $ seti ship:$basename.info.ownerid -1
    } else {
      $ seti ship:$basename.info.ownerid $::abnet::userid
    }
    $ sets ship:$basename.info.author $::abnet::username
    $ setb ship:$basename.info.needs_uploading yes
    $ setb ship:$basename.info.sharing_enabled [$ bool conf.default_share_ships]
    catch { $ remove ship:$basename.info.guid }
    $ sync ship:$basename
    $ sync hangar
    $ sets edit.mountname ship:$basename
    $ sets edit.status_message "[_ A editor fork_successful] $basename"
    $ setb edit.modified no
  }

  method setName nam {
    $ sets [$ str edit.mountname].info.name $nam
    $ setb edit.modified yes
    return yes
  }

  method setReinforcement rein {
    $ setf [$ str edit.mountname].info.reinforcement $rein
    $ setb edit.modified yes
    $editor manip reloadShip
  }

  method enableSharing {} {
    $ setb [$ str edit.mountname].info.sharing_enabled yes
  }
  method disableSharing {} {
    $ setb [$ str edit.mountname].info.sharing_enabled no
  }

  method changeClass cls {
    $editor manip pushUndo
    set oldClass [$ str [$ str edit.mountname].info.class]
    $ sets [$ str edit.mountname].info.class $cls
    set err [$editor manip reloadShip]
    if {[string length $err]} {
      # Couldn't change
      $editor manip popUndo
      $editor manip reloadShip
      $ sets edit.status_message [format [_ A editor change_class_fail] $cls]
      setbar prop
    } else {
      $ sets edit.status_message [format [_ A editor change_class_success] $cls]

      # Move between generic hangars
      set oldClass [string tolower $oldClass]
      set cls [string tolower $cls]
      set name [shipMount2Name [$ str edit.mountname]]
      $ appends hangar.$cls $name
      for {set i 0} {$i < [$ length hangar.$oldClass]} {incr i} {
        if {[$ str hangar.$oldClass.\[$i\]] == $name} {
          $ remix hangar.$oldClass $i
          break
        }
      }
    }
  }

  method fmtlbl {p args} {
    $p add [new ::gui::Label [format {*}$args] left]
  }
}

# Utility proc for some of the sidebars. Takes a panel and a list of
# words, and adds left-justified labels that contain the words, such
# that no label is longer than 24 characters long.
# This assumes that no individual word is longer than 24; if not, some
# lines will be longer.
proc addLabelsWithWordBreaks {panel text} {
  set acc {}
  foreach word $text {
    if {[string length "$acc $word"] > 24 && [string length $acc]} {
      $panel add [new ::gui::Label $acc left]
      set acc {}
    }
    if {[string length $acc]} {
      set acc "$acc $word"
    } else {
      set acc $word
    }
  }

  if {[string length $acc]} {
    $panel add [new ::gui::Label $acc left]
  }
}
