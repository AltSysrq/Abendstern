# Defines procs used to make safe slave interpreters
# truly safe.

# Rename could potentially be used to replace a bridge
# function and learn the magic cookie that gives access
# to C++ memory management. Forbid using rename from or
# to any name containing "c++"
proc safe_rename {i from to} {
  if {[string first {c++} $from] != -1
  ||  [string first {c++} $to] != -1} {
    error "Invalid rename"
  }

  interp invokehidden $i _rename $from $to
}

# We need to prevent proc from stomping on anything
# that already exists.
proc safe_proc {i name ags bod} {
  if {[interp eval $i info exists $name]} {
    error "Duplicate proc"
  }
  interp invokehidden $i _proc $name $ags $bod
}

# Safens the specified slave interpreter
proc make_slave_safe i {
  interp hide $i proc _proc
  interp hide $i rename _rename
  interp alias $i proc {} safe_proc $i
  interp alias $i rename {} safe_rename $i

  # Set resource restrictions
  interp recursionlimit $i 1024
}

# Updates the current time limit of the slave.
proc slave_ping i {
  interp limit $i time -seconds [clock add [clock seconds] 2 seconds]
}
