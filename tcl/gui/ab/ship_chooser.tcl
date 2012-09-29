  # ShipChooser allows the user to select a Ship from a libconfig
  # catalogue. The current ship, along with some info, is shown
  # in the dialogue.
  # Stats for power and capacitance are the raw values divided
  # by the number of cells in the ship.
  # Whenever the ship is changed, the provided action is called,
  # with the new index appended.
  # At the bottom is a search field; when the user types into
  # this field, only ships whose names contain that string
  # (case insensitive) will be displayed, unless no ships do,
  # in which case the current ship cannot be changed
  class ShipChooser {
    inherit BorderContainer

    variable catalogue
    variable index
    variable display
    variable ship
    variable field

    variable ssgGraph
    variable action

    variable nameLabel
    variable authorLabel
    variable searchField
    variable prevSearchText

    constructor {cat act {init 0}} {
      BorderContainer::constructor
    } {
      set catalogue $cat
      set index $init
      set action $act
      set display [new ::gui::SimpleShipDisplay]
      set field [new GameField default 1 1]
      set ship ""

      set selector [new ::gui::BorderContainer]
      set leftb  [new ::gui::Button "<<" "$this previous"]
      set rightb [new ::gui::Button ">>" "$this next"]
      $leftb setLeft
      $rightb setRight
      $selector setElt left $leftb
      $selector setElt right $rightb
      set nameLabel [new ::gui::Label "NAME HERE" centre]
      $selector setElt centre $nameLabel
      setElt top $selector

      set main [new ::gui::StackContainer]
      $main add $display
      set labelPan [new ::gui::VerticalContainer]
      set authorPan [new ::gui::HorizontalContainer]
      $authorPan add [new ::gui::Label "[_ A editor author]: " left]
      set authorLabel [new ::gui::Label "AUTHOR HERE" left]
      $authorPan add $authorLabel
      $labelPan add $authorPan
      set ssgGraph [new ::gui::ShipSpiderGraph]
      $labelPan add $ssgGraph
      $main add $labelPan
      setElt centre $main

      set searchField [new ::gui::TextField [_ A ship_chooser filter] ""]
      set prevSearchText ""
      setElt bottom $searchField

      loadShip
    }

    method reset {cat {ix 0}} {
      set catalogue $cat
      set index $ix
      loadShip
    }

    destructor {
      if {[string length $ship]} {delete object $ship}
      delete object $field
    }

    method draw {} {
      # Before drawing, check to see if the filter text excludes the
      # current ship
      if {$prevSearchText != [$searchField getText]} {
        catch {
          set shipName [$ str [getAt $index].info.name]
          set filter [$searchField getText]
          set prevSearchText $filter
          if {[string length $filter]
          &&  -1 == [string first [string tolower $filter] [string tolower $shipName]]} {
            next
          }
        }
      }

      ::gui::Container::draw
    }

    method getAt ix {
      set mp [$ str $catalogue.\[$ix\]]
      if {-1 == [string first : $mp]} {
        set mp ship:$mp
      }
      return $mp
    }

    method loadShip {} {
      if {[string length $ship]} {delete object $ship}
      if {[catch {
        set ship ""
        set mp [getAt $index]
        set ship [::loadShip $field $mp]
        $display setShip $ship

        $ship configureEngines yes no 1.0

        $ssgGraph setShip $ship

        $nameLabel setText [$ str $mp.info.name]
        if {[$ exists $mp.info.author]} {
          $authorLabel setText [$ str $mp.info.author]
        } else {
          $authorLabel setText [_ A gui unknown]
        }
      } err]} {
        # If there is no ship in the current catalogue,
        # the puts command will fail, as $mp will not be defined
        if {[catch {
          #puts "$mp: $err"
          set err "$mp: $err"
        }]} {
          set err [_ A ship_chooser no_ships]
        }
        $nameLabel setText "\a\[(danger)$err\a\]"
        $authorLabel setText "---"
        $display setShip error
      }
    }

    method getNameSafe root {
      set ret {}
      catch {
        set ret [$ str $root.info.name]
      } err
      return $ret
    }

    method previous {} {
      # If there are zero or one ships in the catalogue,
      # do nothing. Otherwise, cycle until we encounter
      # something different or hit the original index
      if {[$ length $catalogue] <= 1} return

      set origix $index
      set first yes
      while {$first ||
             ($origix != $index
           && ([$ str $catalogue.\[$origix\]] == [getAt $index]
            || ([string length [$searchField getText]]
              && -1 == [string first \
                        [string tolower [$searchField getText]] \
                        [string tolower [getNameSafe [getAt $index]]]])))} {
        set index [expr {($index-1)%[$ length $catalogue]}]
        set first no
      }
      eval "$action $index"
      loadShip
    }

    method next {} {
      # If there are zero or one ships in the catalogue,
      # do nothing. Otherwise, cycle until we encounter
      # something different or hit the original index
      if {[$ length $catalogue] <= 1} return

      set origix $index
      set first yes
      while {$first ||
             ($origix != $index
           && ([$ str $catalogue.\[$origix\]] == [getAt $index]
            || ([string length [$searchField getText]]
              && -1 == [string first \
                        [string tolower [$searchField getText]] \
                        [string tolower [getNameSafe [getAt $index]]]])))} {
        set index [expr {($index+1)%[$ length $catalogue]}]
        set first no
      }
      eval "$action $index"
      loadShip
    }
  }
