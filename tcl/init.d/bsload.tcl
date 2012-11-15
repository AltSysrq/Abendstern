# Loads the AI best ships and creates a hangar for them.
# It only runs on the first login.
if {![info exists ::bsload::index]} {
  namespace eval bsload {
    set index {}
    set filenames $::bsdownld::filenames
    set WEIGHTS {1 3 4 2 1 1}

    proc callback {} {
      global ::bsload::index ::bsload::filenames
      for {set times 0} {$times < 16} {incr times} {
        set filename [lindex $filenames $index]
        set ok no
        catch {
          set conffile [homeq hangar/0/$filename]
          if {![file exists $conffile]} {
            error "$filename does not exist"
          }
          $ openLazily $conffile ship:bs/$filename
          set ok yes
        }

        if {$ok} {
          set weight [lindex $::bsload::WEIGHTS [string index $filename end-1]]
          set class [string index $filename end-2]
          set hangar hangar.user.bs$class.contents
          for {set w 0} {$w < $weight} {incr w} {
            set ix [$ length $hangar]
            set entry $hangar.\[$ix\]
            $ append $hangar STGroup
            $ adds $entry target bs/$filename
            $ addi $entry weight 1
          }
        }

        incr index
        if {$index == [llength $filenames]} {
          return 200
        }
      }
      return [expr {int(100.0*$index/[llength $filenames])}]
    }
  }

  # Remove the existing bestship hangars and create new ones
  foreach class {A B C} {
    catch {
      $ remove hangar.user.bs$class
    }

    $ add hangar.user bs$class STGroup
    $ addb hangar.user.bs$class deletable no
    $ adds hangar.user.bs$class name "AI-$class"
    $ add hangar.user.bs$class contents STList
  }

  set ::bsload::index 0
  $state setCallback [_ A boot bsload] ::bsload::callback
}
