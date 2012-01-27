# This file contains the httpa namespace, which is a drop-in
# replacement for the standard HTTP and provides truely
# asynchronous HTTP operations by running the ::http operations
# in the background thread (since the DNS query and connection
# setup are synchronous).
# It provides a front-end for all procs actually used by Abendstern.
# It requires all requests to be asynchronous (for synchronous,
# just use the standard ::http package)
#
# Note that NUL bytes are substituted with U+0100 (which gets written
# as 00 in binary mode) within the body, so that binary payloads survive
# and do not get truncated (by treatment as C-strings)

if {$THREAD == "foreground"} {
  namespace eval ::httpa {
    # Sets the ::http settings for this interpreter as well as the
    # background thread
    proc config args {
      ::http::config {*}$args
      bkg_req [list ::http::config {*}$args]
    }

    set nextID 0

    # Begins an HTTP query. The following arguments are understood
    # and translated to work as expected:
    #   -command
    #   -progress
    proc geturl {url args} {
      # Start by generating a new ID
      set id "httpa#$::httpa::nextID"
      incr ::httpa::nextID

      set rargs {}

      foreach {argname argval} $args {
        switch -exact -- $argname {
          -command {
            set argval [list ::httpa::command $argval $id]
          }
          -progress {
            set argval [list ::httpa::progress $argval $id]
          }
        }
        lappend rargs $argname $argval
      }

      bkg_req [list ::httpa::geturl $url $id $rargs]

      return $id
    }

    # Aborts an HTTP query.
    proc reset args {
      bkg_req [list ::httpa::reset {*}$args]
    }

    # Performs registration with both threads
    proc register args {
      ::http::register {*}$args
      bkg_req [list ::http::register {*}$args]
    }

    # Used as a callback from the other thread.
    # Sets the state array up, as ::httpa::<tok>
    set STATE_ARRAY_ELTS [list body charset coding currentsize error http \
                               meta posterror status totalsize type url]
    proc importState [list id {*}$STATE_ARRAY_ELTS] {
      foreach var $::httpa::STATE_ARRAY_ELTS {
        set ::httpa::${id}($var) [set $var]
      }
    }

    foreach {pname fname} {data body error error status status code http size currentsize meta meta} {
      proc $pname id "set ::httpa::\${id}($fname)"
    }
    proc ncode id {
      string range [set ::httpa::${id}(http)] 9 11
    }

    proc cleanup id {
      unset ::httpa::$id
      bkg_req [list ::httpa::cleanup $id]
    }
  }
} else {
  # Background side
  namespace eval ::httpa {
    # We use this solely to find the ::http token we'll be using for this,
    # and create mappings between both
    proc geturl {url id arg} {
      # We must handle raised errors appropriately
      # (I really don't understand what the authors were
      #  thinking by having async commands raise errors...)
      if {[catch {
        add_bkgu $id

        set tok [::http::geturl $url {*}$arg]
        set ::httpa::tmap($tok) $id
        set ::httpa::imap($id) $tok
        set ::httpa::ready($id) yes
      } err]} {
        # It failed... set a state array up and run the callback
        rem_bkgu $id
        set ::httpa::ready($id) no
        bkg_ans [list ::httpa::importState $id \
                 {} {} {} 0 $err {} $err {} error 0 {} $url]
        foreach {argn argval} $arg {
          if {$argn == "-command"} {
            eval $argval \{\}
          }
        }
      }
    }

    # Callback to forward progress calls to the foreground thread
    proc progress {callback id tok tot curr} {
      bkg_ans [concat $callback $tok $tot $curr]
    }

    # Callback to send state information and then forward to the
    # original callback
    # This may also be called with an empty tok in case of error
    proc command {callback id tok} {
      if {[string length $tok]} {
        # Update no longer must be called
        rem_bkgu $id

        upvar #0 $tok state
        # We need to keep track of the dummies we added, since
        # it will call error if state(error) exists
        set to_delete {}
        foreach sub {body charset coding currentsize error
                     http meta posterror status totalsize type url} {
          if {![info exists state($sub)]} {
            set state($sub) {}
            lappend to_delete $sub
          }
        }

        # Propagate state
        # Note how \0 is replaced with U+0100 in the body (see comments at
        # top of file)
        bkg_ans [list ::httpa::importState $id \
                [string map [list \0 [format %c 256]] $state(body)] \
                $state(charset) $state(coding) $state(currentsize) \
                $state(error) $state(http) $state(meta) $state(posterror) \
                $state(status) $state(totalsize) $state(type) $state(url)]

        foreach sub $to_delete {
          unset state($sub)
        }
      }
      # Call callback
      bkg_ans [concat $callback $id]
    }

    proc reset {id args} {
      if {![info exists ::httpa::ready($id)]} {
        # We must wait until the connection is established to
        # kill it
        # This will unfortunately block further interthread
        # communication until the operation completes
        vwait ::httpa::ready($id)
      }
      if {$::httpa::ready($id)} {
        ::http::reset $::httpa::imap($id) {*}$args
      }
    }

    proc cleanup id {
      if {[info exists ::httpa::imap($id)]} {
        set tok $::httpa::imap($id)
        ::http::cleanup $tok
        unset ::httpa::tmap($tok)
        unset ::httpa::imap($id)
      }
      unset ::httpa::ready($id)
    }
  }
}
