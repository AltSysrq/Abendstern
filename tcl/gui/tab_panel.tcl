  # The TabPanel is an AWidget that holds one or more AWidgets, each
  # assigned names. Only one of these AWidgets is shown at a time;
  # the user can select them via the buttons to the left.
  # A TabPanel may also have its tabs along the top, by passing the tabsOnTop
  # parameter to the constructor as true.
  class TabPanel {
    inherit BorderContainer

    variable buttonBar
    variable viewPane

    constructor {{tabsOnTop no}} {
      BorderContainer::constructor 0.01
    } {
      if {$tabsOnTop} {
        set buttonBar [new ::gui::HorizontalContainer 0 left]
        setElt top $buttonBar
      } else {
        set buttonBar [new ::gui::VerticalContainer 0 grid]
        setElt left $buttonBar
      }
      set viewPane [new ::gui::TabPanelViewer]
      setElt centre $viewPane
    }

    # Adds a new tab
    method add {name content} {
      $buttonBar add [new ::gui::TabPanelButton $name $viewPane $content]
      $viewPane add $content
    }

    # Returns the currently active component
    method getActive {} {
      $viewPane getActive
    }

    method setActive which {
      [lindex [$buttonBar getMembers] $which] action
    }
  }

  # For internal use by TabPanel.
  # A simple AWidget that forwards everything to its active
  # item. The minWidth and minHeight are the maxima of its
  # contents.
  class TabPanelViewer {
    inherit AWidget

    variable members
    variable active
    constructor {} {
      AWidget::constructor
    } {
      set members {}
    }

    method minWidth {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minWidth])}]
      }
      return $max
    }
    method minHeight {} {
      set max 0
      foreach mem $members {
        set max [expr {max($max,[$mem minHeight])}]
      }
      return $max
    }

    method isFocused {} {
      $active isFocused
    }

    method getAccelerator {} {
      $active getAccelerator
    }

    method setAccelerator p {
      $active setAccelerator $p
    }

    method setSize {a b c d} {
      AWidget::setSize $a $b $c $d
      $active setSize $a $b $c $d
    }

    method revert {} {
      foreach mem $members {
        $mem revert
      }
    }

    method save {} {
      foreach mem $members {
        $mem save
      }
    }

    method draw {} {
      $active draw
    }

    method keyboard evt {
      $active keyboard $evt
    }

    method motion {x y states} {
      $active motion $x $y $states
    }

    method button {evt x y} {
      $active button $evt $x $y
    }

    method character evt {
      $active character $evt
    }

    method getActive {} { return $active }
    method setActive a {
      set active $a
      global currentMode
      $currentMode refreshAccelerators
      $active setSize $left $right $top $bottom
    }

    method setActiveAt a {
      setActive [lindex $members $a]
    }

    method add a {
      lappend members $a
      if {1 == [llength $members]} {
        set active $a
      }
    }

    destructor {
      foreach mem $members {
        delete object $mem
      }
    }
  }

  # For private use by TabPanel.
  # Extends button to use highlighting to indicate
  # current tab.
  class TabPanelButton {
    inherit Button
    variable activatee
    variable pane

    constructor {name pan content} {
      Button::constructor $name "$pan setActive $content"
    } {
      set activatee $content
      set pane $pan
    }

    method draw {} {
      if {$activatee == [$pane getActive]} {set highlighting 1}
      Button::draw
    }
  }

