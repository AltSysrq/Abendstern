# Primary driver file for background processing

package require http
package require tls

set THREAD background

set background_update_tasks {}

# If there is a pending task that requires running update,
# it should be added to the global list with this function.
proc add_bkgu name {
  global background_update_tasks
  lappend background_update_tasks $name
}

# Completed update tasks must be removed with this function.
# If they are not removed, the background thread will continue
# to spin, using 100% CPU time (on that core), as it must alternate
# between update and bkg_rcv.
proc rem_bkgu name {
  global background_update_tasks
  set ix [lsearch $background_update_tasks $name]
  if {$ix != -1} {
    set background_update_tasks [lreplace $background_update_tasks $ix $ix]
  }
}

# Source libraries here
source tcl/httpa.tcl

# For the rest of eternity, process incomming tasks.
# If background_update_tasks is empty, wait by calling
# bkg_wait. Outside of that, just call update and
# evaluate bkg_rcv.
proc main {} {
  global background_update_tasks

  while true {
    update
    while {[string length [set msg [bkg_rcv]]]} {
      namespace eval :: $msg
    }

    if {0 == [llength $background_update_tasks]} {
      bkg_wait
    }
  }
}

# Notify the main thread we're done loading
bkg_ans {set bkgready yes}

main
