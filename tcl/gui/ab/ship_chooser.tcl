# The ShipChooser allows the user to select a ship, filtering by name, author,
# class, category, and user hangar.
class ShipChooser {
  inherit BorderContainer

  variable action
  variable filteredList
  variable ssgGraph
  variable field
  variable ship
  variable display

  variable lstShips
  variable lstAuthors
  variable lstClasses
  variable lstCategories
  variable lstHangars
  variable lname
  variable lauthor
  variable fsearch

  # Creates a new ShipChooser.
  # action: Script to run when the ship selection is changed; has the root of
  #         the ship appended.
  # classes: A list of classes to allow the user to select; {} indicates the
  #          "all classes" option.
  # hangarActions: Not yet implemented
  # additionalComponents: Components to add to the bottom pane
  constructor {
    action_
    {classes {{} A B C}}
    {hangarActions {}}
    {additionalComponents {}}
  } {
    BorderContainer::constructor
  } {
    global ::ship_index::fullList
    lappend ::ship_index::choosers $this

    if {$fullList eq {}} {
      set fullList [generate-ship-list]
    }
    set action $action_
    set display [new ::gui::SimpleShipDisplay]
    set field [new GameField default 1 1]
    set ship ""

    set lstShips [new ::gui::List \
                      [_ A shipbrowse shiplist] {} no \
                      [list $this selection-changed]]
    set lstAuthors [new ::gui::List \
                        [_ A editor author] [get-author-list] yes \
                        [list $this filter-changed]]
    set lstClasses [new ::gui::List \
                        [_ A editor ship_class] [get-class-list $classes] yes \
                        [list $this filter-changed]]
    set lstCategories [new ::gui::List \
                           [_ A editor category] [get-category-list] yes \
                           [list $this filter-changed]]
    set lstHangars [new ::gui::List \
                        [_ A hangar in_hangar] [get-hangar-list] yes \
                        [list $this filter-changed]]
    set lname [new ::gui::Label ""]
    set lauthor [new ::gui::Label ""]
    set fsearch [new ::gui::TextField [_ A ship_chooser filter] "" {} {} {} \
                     {} [list $this filter-changed]]
    set ssgGraph [new ::gui::ShipSpiderGraph]

    setElt left $lstShips
    set filters [new ::gui::VerticalContainer 0.01 grid]
    $filters add $lstAuthors
    $filters add $lstClasses
    $filters add $lstCategories
    $filters add $lstHangars
    setElt right $filters

    set topmatter [new ::gui::VerticalContainer 0.01]
    $topmatter add $lname
    $topmatter add $lauthor
    $topmatter add $ssgGraph
    setElt top $topmatter

    setElt centre $display

    set bottommatter [new ::gui::VerticalContainer 0.01]
    $bottommatter add $fsearch
    foreach c $additionalComponents {
      $bottommatter add $c
    }
    setElt bottom $bottommatter

    filter-changed
  }

  destructor {
    set ix [lsearch -exact $::ship_index::choosers $this]
    set ::ship_index::choosers [lreplace $::ship_index::choosers $ix $ix]
    del $ship $field
  }

  method selected-properties {lst} {
    set selection [$lst getSelection]
    if {[llength $selection] != 0 &&
        [llength $selection] != [llength [$lst getItems]]} {
      set selected {}
      set items [$lst getItems]
      foreach ix $selection {
        lappend selected [lindex $items $ix]
      }

      set ret {}
      foreach str $selected {
        lappend ret [string range $str 2+[string first "\a\a" $str] end]
      }

      return $ret
    } else {
      return {}
    }
  }

  method get-ship-dict {rt} {
    set s {}
    # Get ship information, and stringprep its name
    dict set s name [$ str $rt.info.name]
    dict set s spname [::stringprep::stringprep basic [$ str $rt.info.name]]
    dict set s author [$ str $rt.info.author]
    dict set s class [$ str $rt.info.class]
    dict set s category [Ship_categorise $rt]
    dict set s hangars {}
    dict set s root $rt
    return $s
  }

  method generate-ship-list {} {
    set lst {}
    conffor entry hangar.all_ships {
      if {[catch {
        set rt [shipName2Mount [$ str $entry]]
        lappend lst [get-ship-dict $rt]
      } err errinfo]} {
        log "$entry: $err"
        # Tcl for some reason escapes the error info with backslashes instead
        # of using braces like list does, so convert both ways so the result is
        # readable.
        log [list {*}$errinfo]
      }
    }

    lsort -command ::gui::compare-ship-dict-spname $lst
  }

  method add-mount {root} {
    global ::ship_index::fullList
    lappend fullList [get-ship-dict $root]
    set fullList \
        [lsort -command ::gui::compare-ship-dict-spname $fullList]
  }

  method get-author-list {} {
    global ::ship_index::fullList
    set authors {}
    foreach s $fullList {
      set a [dict get $s author]
      lappend authors "$a\a\a[string tolower $a]"
    }

    lsort -dictionary -nocase -unique $authors
  }

  method get-class-list {classes} {
    set lst {}
    foreach class $classes {
      if {$class ne ""} {
        lappend lst [format "%s%s\a\a%s" [_ A editor class_prefix] \
                         $class $class]
      } else {
        lappend lst [format "%s\a\a*" [_ A misc class_all_ships]]
      }
    }

    return $lst
  }

  method get-category-list {} {
    set lst {}
    foreach cat {Swarm Interceptor Fighter Attacker Subcapital Defender} {
      lappend lst [format "%s\a\a%s" \
                       [_ A editor "cat[shipCategoryToInt $cat]"] $cat]
    }

    return $lst
  }

  method get-hangar-list {} {
    set lst {}
    conffor hangar hangar.user {
      lappend lst [format "%s\a\a%s" \
                       [$ str $hangar.name] $hangar]
    }

    lsort -dictionary -nocase $lst
  }

  method filter-changed args {
    global ::ship_index::fullList ::ship_index::hangarsIndexed
    set filteredList {}
    set classes [selected-properties $lstClasses]
    if {-1 != [lsearch -exact $classes *]} {
      set classes {}
    }
    set authors [selected-properties $lstAuthors]
    set categories [selected-properties $lstCategories]
    set hangars [selected-properties $lstHangars]

    # Index any hangars which have not yet been indexed
    foreach hangar $hangars {
      if {![dict exists $hangarsIndexed $hangar]} {
        index-hangar $hangar
      }
    }

    # Loop through the full list, adding ships which pass all the filters.
    foreach s $fullList {
      if {$classes ne {} &&
          -1 == [lsearch -exact $classes [dict get $s class]]} {
        continue
      }

      if {$categories ne {} &&
          -1 == [lsearch -exact $categories [dict get $s category]]} {
        continue
      }

      if {$authors ne {} &&
          -1 == [lsearch -sorted -exact -dictionary -nocase $authors \
                     [string tolower [dict get $s author]]]} {
        continue
      }

      if {$hangars ne {}} {
        set inAnyHangar no
        foreach hangar $hangars {
          if {[dict exists $s hangars $hangar]} {
            set inAnyHangar yes
            break
          }
        }
        if {!$inAnyHangar} continue
      }

      set fname [string trim [$fsearch getText]]
      if {$fname ne "" &&
          -1 == [string first [::stringprep::stringprep basic $fname] \
                     [dict get $s spname]]} {
        continue
      }

      # All applied filters match
      lappend filteredList $s
    }

    # Get the currently-preferred ships to recreate the selection
    set preferredMain [$ str conf.preferred.main]
    set preferredC [$ str conf.preferred.C]
    set preferredB [$ str conf.preferred.B]
    set preferredA [$ str conf.preferred.A]

    set preferredIx {}
    # Convert to list items
    set items {}
    foreach s $filteredList {
      set rt [dict get $s root]
      if {$rt eq $preferredMain} {
        set preferredIx [llength $items]
      } elseif {$preferredIx eq {} &&
                ($rt eq $preferredC ||
                 $rt eq $preferredB ||
                 $rt eq $preferredA)} {
        set preferredIx [llength $items]
      }

      lappend items [dict get $s name]
    }

    # If there are any items, but no preferred selection, select the first item
    if {$items ne {} && $preferredIx eq {}} {
      set preferredIx 0
    }

    $lstShips setItems $items
    $lstShips setSelection $preferredIx
    selection-changed
  }

  method index-hangar {hangar} {
    global ::ship_index::fullList ::ship_index::hangarsIndexed
    set contained {}
    conffor entry $hangar.contents {
      lappend contained [shipName2Mount [$ str $entry.target]]
    }

    set contained [lsort $contained]

    set oldFullList $fullList
    set fullList {}
    foreach s $oldFullList {
      set rt [dict get $s root]

      if {-1 != [lsearch -sorted -exact $contained $rt]} {
        set h [dict get $s hangars]
        dict set h $hangar {}
        dict set s hangars $h
      }

      lappend fullList $s
    }
  }

  method selection-changed args {
    global ::ship_index::fullList ::ship_index::hangarsIndexed
    set sel [$lstShips getSelection]
    if {$sel eq {}} {
      del $ship
      set ship {}
      $display setShip {}

      $lname setText [_ A ship_chooser no_ships]
      $lauthor setText ""
    } else {
      set s [lindex $filteredList $sel]
      # Do nothing if a ship is loaded and is rooted here
      if {$ship ne {} &&
          [$ship cget -typeName] eq [dict get $s root]} {
        return
      }

      del $ship
      set ship {}

      # Try to load the ship
      if {[catch {
        set ship [loadShip $field [dict get $s root]]
      } err errinfo]} {
        # Something went wrong!
        log "Error loading ship $s: $err"
        # Remove the ship from the full list and reset
        set ix [lsearch -exact $fullList $s]
        set fullList [lreplace $fullList $ix $ix]
        filter-changed
        return
      }

      # OK
      $display setShip $ship
      $lname setText [dict get $s name]
      $lauthor setText [dict get $s author]
      $ssgGraph setShip $ship

      # Update preferred selections
      $ sets conf.preferred.main [dict get $s root]
      $ sets conf.preferred.[dict get $s class] [dict get $s root]

      # Notify the callback, if requested
      if {$action ne {}} {
        namespace eval :: [list {*}$action [dict get $s root]]
      }
    }
  }

  # Returns the root of the currently-selected ship, or the empty string if
  # there is no selection.
  method get-selected {} {
    set sel [$lstShips getSelection]
    if {$sel eq {}} {
      return {}
    } else {
      return [dict get [lindex $filteredList $sel] root]
    }
  }
}

proc compare-ship-dict-spname {a b} {
  ::stringprep::compare basic \
      [dict get $a spname] \
      [dict get $b spname]
}
