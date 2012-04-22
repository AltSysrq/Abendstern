# Provides facilities to generate procs to validate the format of complex
# dictionaries, as are used in the datp and dats fields in BasicGame, for
# example.
#
# A schema is described by a sequence of schema descriptions. A schema
# description is a list with an even number of elements; each even element is
# a type of requirement, and the following is its parameter.
#
# Each requirement operates solely on a "current element", though some may also
# access other parts of the tree.
#
# Requirements may either adjust their values to be acceptable (a silent
# failure, though a note is logged), or, if no reasonable value can be
# determined, fail critically, indicating that the data is unusable.
#
# Requirements:
#   subdir 2LIST
#     Verifies that the current element is usable as a dictionary. The parameter
#     is a list with an even number of elements; each even element is the name
#     of a child of the current element, and the following is a list of
#     requiremnets to apply to the child (with the child as the current
#     element). Children not listed are silently ignored.
#     If a listed child begins with '+', the rest of the child is the child's
#     actual name, and the following element is used as a default if the child
#     does not exist (silent failure).
#     If a listed child begins with '?', it is not an error if it does not
#     exist; simply, no action is taken in this case.
#     If the current element is not a dictionary, a critical failure results.
#     If a listed child does not exist, critical failure occurs (unless a
#     default is given with the + syntax).
#   within 2LIST
#     Like subdir, but does not verify that the current element is usable as a
#     dictionary. This is used to "append" requirements to a previous subdir
#     requirement.
#   foreign-key PATH
#     Requires that an key equal to this element exist under PATH, which is
#     relative to the root.
#     If this requirement is not met, critical failure results.
#   foreign-req REQS
#     Used immediately after a foreign-key requirement. Sets the current element
#     to the item indicated by the foreign key, and applies REQS to it.
#   list REQS
#     Requires that the current element is a valid list. The REQS are a set of
#     requirements to apply to each element within the list.
#     If the element is not a valid list, critical failure results.
#   llmin INT
#     Requires that the length of the current element, assumed to be a list, is
#     at least INT long (inclusive).
#     If this requirement is not met, critical failure results.
#   llmax INT | +INT
#     Requires that the length of the current element, assumed to be a list, is
#     at most INT long (inclusive).
#     If this requirement is not met, and INT begins with a '+', the list is
#     trimmed to be INT long, resulting in a silent failure. Otherwise, when the
#     requirement is not met, critical failure results.
#   integer DEFAULT
#     Requires that the current element is an integer, according to
#     string is integer -strict
#     Failure is a critical failure, unless DEFAULT is a non-empty string, in
#     which case failure is a silent failure setting the value to DEFAULT.
#   float DEFAULT
#     Like integer, but with floats. Also requires that the value is finite and
#     non-NaN.
#   int10 DEFAULT
#     Like integer, but only allows decimal unsigned integers (ie, [0-9]+).
#   bool DEFAULT
#     Like integer, but with booleans.
#   nmin MIN
#     Requires the current element, which is a number, to be at least MIN,
#     inclusive. Failure is a silent failure adjusting the value to MIN.
#   nmax MAX
#     Requires the current element, which is a number, to be at most MAX,
#     inclusive. Failure is a silent failure adjusting the value to MAX.
#   strmin LEN | {LEN DEFAULT}
#     Requires the string length of the current element to be at least LEN,
#     inclusive. If it is not, and no default is given, critical failure
#     results; if it is not, but a default is given, a silent failure occurs,
#     replacing the string with DEFAULT.
#   strmax LEN | +LEN
#     Requires the string length of the current element to be at most LEN,
#     inclusive. If it is not, critical failure results, unless LEN is prefixed
#     with '+', which trims the string to the first LEN characters.
#   string CAT | {CAT DEFAULT}
#     Requires [string is CAT] to be true for the current element. If DEFAULT
#     is given, failure is silent, replacing the datum with the given.
#     Otherwise, failure is critical.
#   nchar CAT | +CAT
#     Requires that no characters of category CAT (for [string is CAT]) are
#     present in the string representation of the current element. If any are
#     found, critical failure results, unless the + prefix is given, which
#     simply strips the characters from the string (silent failure).
#   trimmed SILENT
#     Ensure that the string is trimmed. If SILENT is true, failure simply trims
#     the string; otherwise, failure is critical.
#   verbatim CODE
#     The Tcl code CODE is used to examine the current element. It has access
#     to the following variables (which it shouldn't change, other than possibly
#     root):
#       root    The root dictionary being validated
#       curr    The current element
#       path    The current path
#       stack   Stack (list, elements pushed/popped to/from end) of
#               {$curr $path} pairs used for subordinate evaluation.
#     Critical failure is indicated by returning false.
#   aux NAME
#     Does nothing in and of itself; rather, it marks a continuation point where
#     more requirements can be inserted later, in case the current element is
#     nested within a foreign key relation, for example.
#   aux-NAME REQS
#     The REQS are added to the hook with name NAME instead of on the current
#     element.
#     This MUST occur as a top-level requirement.
#   rem ...
#     Does nothing
namespace eval ::schema {
  # Set to true if schema violations should be logged
  set verbose no

  set nextSymbol 0

  # Resets or prepares the schema validator generator for a new schema.
  # This must be called prior to any other ::schema procs.
  proc init {} {
    set ::schema::accum {}
    set ::schema::nextSymbol 0
  }

  # Appends the given list to the current schema.
  proc add {schematum} {
    set ::schema::accum [concat $::schema::accum $schematum]
  }

  # Appends the contents of the given file to the current schema.
  proc addFile {filename} {
    set f [open $filename r]
    set schematum [read $f]
    close $f
    add $schematum
  }

  # Defines a proc in the given namespace (default ::) that performs validation
  # according to the defined schema.
  # The proc has the signature
  #   NAME dictvar
  # where dictvar is a variable containing the dictionary to validate
  proc define {name {namespace ::}} {
    namespace eval $namespace "
      proc $name {rootvar} {
        upvar \$rootvar root
        set path {}
        set stack {}
        set curr \$root
        set procname {$name}
        [generate-reqs $::schema::accum]
        set root \$curr
        return true
      }"
  }

  proc critfail msg {
    format {::schema::log "$procname: $path: Critical failure: %s"; return false} $msg
  }
  proc silfail msg {
    format {::schema::log "$procname: $path: Warning: %s"} $msg
  }

  proc log {msg} {
    if {$::schema::verbose} {
      ::log $msg
    }
  }

  proc pusha newpath {
    format {
      lappend stack $path $curr
      set path %s
      set curr [dict get $root {*}$path]
    } $newpath
  }
  proc push0 {} {
    return {lappend stack $path $curr}
  }
  proc pushs child {
    format {
      lappend stack $path $curr
      lappend path {%s}
      set curr [dict get $curr {%s}]
    } $child $child
  }
  proc pop {} {
    return {
      set path [lindex $stack end-1]
      set curr [lindex $stack end]
      set stack [lrange $stack 0 end-2]
      if {$path eq {}} {set curr $root}
    }
  }

  proc gensym {} {
    incr ::schema::nextSymbol
    return "x$::schema::nextSymbol"
  }

  proc generate-reqs {schemata} {
    set str {}
    foreach {req parm} $schemata {
      if {![string match aux-* $req]} {
        # Normal requirement
        append str "[gen-$req $parm]\n"
      }
    }
    return $str
  }

  proc gen-subdir {children} {
    return "if {!\[string is list \$curr\]} {
      [critfail "Dictionary has invalid format."]
    }
    if {\[llength \$curr\] & 1} {
      [critfail "Dictionary has odd number of elements."]
    }
    [gen-within $children]"
  }

  proc gen-within {children} {
    set str {}
    foreach {child reqs} $children {
      if {[string index $child 0] eq "+"} {
        set cname [string range $child 1 end]
        append str \
        "if {!\[dict exists \$curr {$cname}\]} {
          [silfail "Missing child, setting default: $cname"]
          dict set curr {$cname} {$reqs}
        }\n"
      } else {
        if {[string index $child 0] eq "?"} {
          set failure {}
          set child [string range $child 1 end]
        } else {
          set failure [critfail "Missing child: $child"]
        }
        append str \
        "if {!\[dict exists \$curr $child\]} {
          $failure
         } else {
          [pushs "$child"]
          [generate-reqs $reqs]
          set v \$curr
          [pop]
          dict set curr {$child} \$v
        }\n"
      }
    }
    return $str
  }

  proc gen-foreign-key {rel} {
    return \
    "if {!\[dict exists \$root {*}{$rel} \$curr\]} {
      [critfail "Foreign-key violation: root $rel"]
    }
    set foreignKeyPath \[list $rel \$curr\]"
  }

  proc gen-foreign-req {reqs} {
    return \
    "[pusha "\$foreignKeyPath"]
    [generate-reqs $reqs]
    dict set root {*}\$foreignKeyPath \$curr
    [pop]"
  }

  proc gen-list {reqs} {
    set counter [gensym]
    return \
    "if {!\[string is list \$curr\]} {
      [critfail "Invalid list format"]
    }
    for {set $counter 0} {$$counter < \[llength \$curr\]} {incr $counter} {
      [push0]
      lappend path $$counter
      set curr \[lindex \$curr $$counter\]
      [generate-reqs $reqs]
      set v \$curr
      [pop]
      lset curr $$counter \$v
    }"
  }

  proc gen-llmin {min} {
    return \
    "if {\[llength \$curr\] < $min} {
      [critfail "List length shorter than $min"]
    }"
  }
  proc gen-llmax {max} {
    if {"+" eq [string index $max 0]} {
      set failure \
      "[silfail "List length greater than $max"]
      set curr \[lrange \$curr 0 $max-1\]"
    } else {
      set failure [critfail "List length greater than $max"]
    }
    return \
    "if {\[llength \$curr\] > $max} {
      $failure
    }"
  }

  proc numeric-failure {type default} {
    if {$default == {}} {
      return [critfail "Bad $type format"]
    } else {
      return \
      "[silfail "Bad $type format"]
      set curr $default"
    }
  }
  proc gen-integer {default} {
    return \
    "if {!\[string is integer -strict \$curr\]} {
      [numeric-failure integer $default]
    }"
  }
  proc gen-float {default} {
    return \
   "if {!\[string is double -strict \$curr\]
    ||  \$curr != \$curr
    ||  \$curr == Inf
    ||  \$curr == -Inf} {
      [numeric-failure float $default]
    }"
  }
  proc gen-int10 {default} {
    return \
   "if {!\[string is ascii -strict \$curr\]
    ||  !\[string is digit -strict \$curr\]} {
      [numeric-failure "base-10 positive integer" $default]
    }"
  }
  proc gen-bool {default} {
    return \
    "if {!\[string is boolean -strict \$curr\]} {
      [numeric-failure "boolean" $default]
    }"
  }
  proc gen-nmin {min} {
    return \
    "if {\$curr < $min} {
      [silfail "Value less than $min"]
      set curr $min
    }"
  }
  proc gen-nmax {max} {
    return \
    "if {\$curr > $max} {
      [silfail "Value greater than $max"]
      set curr $max
    }"
  }

  proc gen-strmin {parms} {
    set min [lindex $parms 0]
    if {[llength $parms] == 2} {
      set failure \
      "[silfail "String length less than $min"]
      set curr {[lindex $parms 1]}"
    } else {
      set failure [critfail "String length less than $min"]
    }
    return \
    "if {\[string length \$curr\] < $min} {
      $failure
    }"
  }
  proc gen-strmax {max} {
    if {"+" == [string index $max 0]} {
      set failure \
      "[silfail "String length greater than $max"]
      set curr \[string range \$curr 0 $max-1\]"
    } else {
      set failure [critfail "String length greater than $max"]
    }
    return \
    "if {\[string length \$curr\] > $max} {
      $failure
    }"
  }

  proc gen-string {parms} {
    set cat [lindex $parms 0]
    if {[llength $parms] == 2} {
      set failure \
      "[silfail "String not in category $cat"]
      set curr {[lindex $parms 1]}"
    } else {
      set failure [critfail "String not in category $cat"]
    }
    return \
    "if {!\[string is $cat \$curr\]} {
      $failure
    }"
  }

  proc gen-nchar {cat} {
    set counter [gensym]
    if {"+" eq [string index $cat 0]} {
      set cat [string range $cat 1 end]
      return \
      "set failed no
      for {set $counter 0} {$$counter < \[string length \$curr\]} \
          {incr $counter} {
        if {\[string is $cat \[string index \$curr $$counter\]\]} {
          set failed yes
          set curr \[string replace \$curr $$counter $$counter\]
        }
      }
      if {\$failed} {
        [silfail "Bad character(s) (category $cat) in string"]
      }"
    } else {
      return \
      "for {set $counter 0} {$$counter < \[string length \$curr\]} \
           {incr $counter} {
        if {\[string is $cat \[string index \$curr $$counter\]\]} {
          [critfail "Bad character(s) (category $cat) in string"]
        }
      }"
    }
  }

  proc gen-trimmed {silent} {
    if {$silent} {
      set failure \
      "[silfail "Non-trimmed string"]
      set curr \$trimmed"
    } else {
      set failure [critfail "Non-trimmed string"]
    }
    return \
    "set trimmed \[string trim \$curr\]
    if {\$trimmed ne \$curr} {
      $failure
    }"
  }

  proc gen-verbatim {code} {
    return $code
  }

  proc gen-aux {label} {
    set label "aux-$label"
    set str {}
    foreach {req body} $::schema::accum {
      if {$req eq $label} {
        append str "[generate-reqs $body]\n"
      }
    }
    return $str
  }

  proc gen-rem {args} { return "" }
}
