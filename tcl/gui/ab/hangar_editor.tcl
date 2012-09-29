  # The HangarEditor is a simple Mode that allows the user to
  # edit a hangar. It uses a simple two-paned design:
  #   Name [................................................]
  #   /-------------\                       /---------------\
  #   |             \      <<<Add>          |               |
  #   |             \      <Remove>>>       |               |
  #   |             \                       |               |
  #   |             \                       |               |
  #   ...
  #   \-------------/                       |               |
  #   Weight <-------->                     \---------------/
  class HangarEditor {
    inherit Mode
    variable nameBox
    variable addedList
    variable availList
    variable weightSlider
    variable catalogue
    variable onReturn

    constructor {cat ret} {
      Mode::constructor
    } {
      set catalogue $cat
      set onReturn $ret
      set nameBox [new ::gui::TextField [_ A editor ship_name] [$ str $cat.name]]
      set addedList [new ::gui::List [_ A hangar in_hangar] {} yes "$this selectionChange"]
      set availList [new ::gui::List [_ A hangar available] {} yes]
      set weightSlider none
      set weightSlider [new ::gui::Slider [_ A hangar weight] \
                        int {expr 1} {} 1 256 1 "$this sliderMoved"]

      # Produce list of ships; format: CLASS: NAME\a\aINTNAME\n
      set ships {}
      for {set i 0} {$i < [$ length hangar.all_ships]} {incr i} {
        set shipName [$ str hangar.all_ships.\[$i\]]
        set mount ship:$shipName
        set cls [$ str $mount.info.class]
        set properName [$ str $mount.info.name]
        lappend ships [format "%s: %s\a\a%s\n" $cls $properName $shipName]
      }

      # For each item already in the contents, add it to the used list
      # and remove from the available list
      set usedShips {}
      for {set i 0} {$i < [$ length $catalogue.contents]} {incr i} {
        set sentinel "\a\a[$ str $catalogue.contents.\[$i\].target]\n"
        # Search for the sentinel
        for {set j 0} \
          {$j < [llength $ships] && -1 == [string first $sentinel [lindex $ships $j]]} \
          {incr j} {}
        # j is now at the correct index
        if {$j < [llength $ships]} {
          lappend usedShips [lindex $ships $j]
          set ships [lreplace $ships $j $j]
        } else {
          # Not a valid ship (??)
          $ remix $catalogue.contents $i
          incr i -1
        }
      }

      set mainPanel [new ::gui::BorderContainer 0 0.02]
      $mainPanel setElt top $nameBox
      set middlePane [new ::gui::VerticalContainer 0.03 centre]
      $middlePane add [new ::gui::Button [_ A hangar add] "$this addSelected"]
      $middlePane add [new ::gui::Button [_ A hangar rem] "$this removeSelected"]
      set leftPane [new ::gui::BorderContainer]
      $leftPane setElt centre $addedList
      $leftPane setElt bottom $weightSlider
      set rightPane [new ::gui::BorderContainer]
      $rightPane setElt centre $availList
      set okCancel [new ::gui::HorizontalContainer 0.02 right]
      set ok [new ::gui::Button [_ A gui ok] "$this okButton"]
      $ok setDefault
      $okCancel add $ok
      set can [new ::gui::Button [_ A gui cancel] "$this cancelButton"]
      $can setCancel
      $okCancel add $can
      $rightPane setElt bottom $okCancel
      $mainPanel setElt centre [new ::gui::DividedContainer $leftPane $middlePane $rightPane 0.03]
      set root [new ::gui::Frame $mainPanel]
      refreshAccelerators
      $root setSize 0 1 $::vheight 0

      $addedList setItems [lsort $usedShips]
      $availList setItems [lsort $ships]
    }


    # Strip the pretty text from a list entry and return the
    # internal ship name
    method deprettyShip text {
      set ix [string last "\a\a" $text]
      set len [string length $text]
      string range $text [expr {$ix+2}] [expr {$len-2}]
    }

    # Searches the catalogue for the given ship name;
    # returns the index of its entry or -1 if it does
    # not exist
    method findCatalogueEntry name {
      for {set i 0} {$i < [$ length $catalogue.contents]} {incr i} {
        if {$name == [$ str $catalogue.contents.\[$i\].target]} {
          return $i
        }
      }

      return -1
    }

    method addSelected {} {
      set sel [$availList getSelection]
      if {0 == [llength $sel]} return

      set items [$availList getItems]
      for {set i 0; set ci [$ length $catalogue.contents]} \
          {$i < [llength $sel]} {incr i; incr ci} {
        set ix [lindex $sel $i]
        set ship [deprettyShip [lindex $items $ix]]
        $ append $catalogue.contents STGroup
        $ adds $catalogue.contents.\[$ci\] target $ship
        $ addi $catalogue.contents.\[$ci\] weight 1
      }

      moveFromTo $availList $addedList
      $weightSlider setValue 1
    }

    method removeSelected {} {
      set sel [$addedList getSelection]
      if {0==[llength $sel]} return

      set items [$addedList getItems]
      for {set i 0} {$i < [llength $sel]} {incr i} {
        set ix [lindex $sel $i]
        set ship [deprettyShip [lindex $items $ix]]
        $ remix $catalogue.contents [findCatalogueEntry $ship]
      }

      moveFromTo $addedList $availList
    }

    # Transfer all selected items from fromList into
    # toList; fromList's selection will be empty, and
    # the moved items will be selected in toList
    method moveFromTo {from to} {
      set sel [$from getSelection]
      set toMove {}
      set froms [$from getItems]
      set tos [$to getItems]
      foreach ix $sel {
        lappend toMove [lindex $froms $ix]
        lappend tos [lindex $froms $ix]
      }
      foreach item $toMove {
        set ix [lsearch -exact $froms $item]
        set froms [lreplace $froms $ix $ix]
      }
      # Froms is still sorted, tos is not
      set tos [lsort $tos]
      # Generate new selection
      set sel {}
      foreach item $toMove {
        lappend sel [lsearch -exact $tos $item]
      }

      # Finish
      $to setItems $tos
      $from setItems $froms
      $to setSelection $sel
    }

    method selectionChange {} {
      set sel [$addedList getSelection]
      # If no selection, nothing to change
      if {0==[llength $sel]} return

      # Set the slider to the first item's weight
      set item [lindex [$addedList getItems] [lindex $sel 0]]
      set ship [deprettyShip $item]
      set ix [findCatalogueEntry $ship]
      $weightSlider setValue [$ int $catalogue.contents.\[$ix\].weight]
    }

    method sliderMoved args {
      if {$weightSlider == "none"} return

      # Change weights for all selected
      set sel [$addedList getSelection]
      set items [$addedList getItems]
      set val [$weightSlider getValue]
      foreach ix $sel {
        set item [lindex $items $ix]
        set ship [deprettyShip $item]
        set index [findCatalogueEntry $ship]
        $ seti $catalogue.contents.\[$index\].weight $val
      }
    }

    method okButton {} {
      $ sets $catalogue.name [$nameBox getText]
      $ sync hangar
      namespace eval :: $onReturn
      lappend ::gui::autodelete $this
    }
    method cancelButton {} {
      $ revert hangar
      namespace eval :: $onReturn
      lappend ::gui::autodelete $this
    }
  }
