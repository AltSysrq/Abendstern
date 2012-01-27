# Provides information on local subscriptions.
# Specifically, it manages the lssinfo.rc file and provides an
# easy to use interface to these data.

if {[catch {
  $ open lssinfo.rc lssinfo
}]} {
  $ create lssinfo.rc lssinfo
}

# Returns the location of the lssinfo setting to store data in.
# If it does not exist, it is created automatically.
proc lssis {} {
  set s lssinfo.u$::abnet::userid
  if {![$ exists $s]} {
    $ add lssinfo u$::abnet::userid STGroup
    $ addi $s upload 0
    $ addi $s download 0
    $ adds $s ssfingerprint ""
    $ adds $s force "" ;# Will store a Tcl list (only Tcl ever accesses it, and only as a list)
  }
  return $s
}

# Returns the most recent time of uploading shared ships.
proc lssi_lastUpload {} {
  $ int [lssis].upload
}

# Sets the most recent time of loading shared ships to
# the current time.
proc lssi_setUpload {} {
  $ seti [lssis].upload [clock seconds]
}

# Returns the most recent time of downloading ships.
proc lssi_lastDownload {} {
  $ int [lssis].download
}

# Sets the most recent time of downloading ships to the current time.
proc lssi_setDownload {} {
  $ seti [lssis].download [clock seconds]
}

# Returns whether the given subscriber info (user subscriptions and
# ship subscriptions) matches what is on record.
proc lssi_subscriptionMatchesRecord {us ss} {
  set fingerprint [list [lsort $us] -- [lsort $ss]]
  expr {$fingerprint == [$ str [lssis].ssfingerprint]}
}

# Sets the subscriber fingerprint to the given information.
proc lssi_updateSubscriptionRecord {us ss} {
  $ sets [lssis].ssfingerprint [list [lsort $us] -- [lsort $ss]]
}

# Returns a list of ships that MUST be downloaded before the
# internal subscription record is accurate.
# The list has the format {shipid fileid owner name}
# (This should only be used if lssi_subscriptionMatchesRecord
#  returns true.)
proc lssi_getForceList {} {
  $ str [lssis].force
}

# Adds the given list of data to the force list.
# (See lssi_getForceList for the data format)
proc lssi_appendForceList {ships} {
  set s [lssis]
  $ sets $s.force [concat [$ str $s.force] $ships]
}

# Clears the force list.
# (See lssi_getForceList)
proc lssi_clearForceList {} {
  $ sets [lssis].force ""
}

# Removes the given list of shipids from the force list.
# (This includes ONLY the shipids.)
proc lssi_removeForceList {shipids} {
  set shipids [lsort $shipids]
  set result {}
  set s [lssis]
  foreach {shipid fileid owner name} [$ str $s.force] {
    if {-1 == [lsearch -sorted -exact $shipids $shipid]} {
      lappend result $shipid $fileid $owner $name
    }
  }
  $ sets $s.force $result
}
