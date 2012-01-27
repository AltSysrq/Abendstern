# Allow the user to select a former state of the ship.
# Clicking on any option in the list immediately applies it.

# Make the list of items;
# each item has the format
#   index+1: description
# Where description is:
#   Current             for the most recent
#   X seconds ago       for less than a minute
#   XX:XX               for one minute to an hour
#   XX:XX:XX            otherwise
set now [SDL_GetTicks]
set historyList {}
for {set i [expr {[$ length edit.history_times]-1}]} {$i >= 0} {incr i -1} {
  if {$i == [$ length edit.history_times]-1} {
    set descr [_ A editor history_current]
  } else {
    set diff [expr {$now-[$ int edit.history_times.\[$i\]]}]
    if {$diff < 60000} {
      set descr [format [_ A editor history_seconds] [expr {$diff/1000}]]
    } elseif {$diff < 3600000} {
      set descr [format [_ A editor history_minutes] \
                        [expr {$diff/60000}] [expr {$diff%60000/1000}]]
    } else {
      set descr [format [_ A editor history_hours] \
                        [expr {$diff/3600000}] \
                        [expr {$diff%3600000/60000}] \
                        [expr {$diff%60000/1000}]]
    }
  }

  lappend historyList "[expr {$i+1}]: $descr"
}
set panel [new ::gui::List "[_ A editor history]                       " $historyList no [format {
  set sel [$::historyPanel getSelection]
  if {[llength $sel]} {
    set sel [lindex $sel 0]
    %s manip revertToHistory [expr {[$ length edit.history_times]-1-$sel}]
  }
} $editor] ]
set ::historyPanel $panel