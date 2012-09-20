# Provides functions to manage accounts on the Abendstern Network.

package require aes

namespace eval ::abnet {
  # The server to connect to
  # This may be changed to an explicit IP address in case
  # of the lanpatch kicking in
  set SERVER abendstern.servegame.com
  # The port to connect to for the Abendstern Network protocol
  set PORT 12545
  # The version of the Abendstern Network protocol in use
  set NETWVERS 0
  set f [open version r]
  # The version of Abendstern running
  set ABVERS $::VERSION
  ::http::config  -useragent "Abendstern $ABVERS"
  ::httpa::config -useragent "Abendstern $ABVERS"
  close $f
  unset f

  # The maximum time to spend in the busy state before timing out,
  # in seconds
  set MAXIMUM_BUSY_TIME 10
  # The maximum interval between messages (to fill with ping if
  # nothing else to send), in seconds
  set MAXIMUM_MSG_INTERVAL 60
  # Block size of AES
  set BLOCK_SZ 16

  # The Diffie-Hellman Key Exchange parms
  set DHKE_BASE 5
  set DHKE_PRIME_MODULUS 444291e51b3ea5fd16673e95674b01e7b
  crypto_init $DHKE_PRIME_MODULUS

  # Whether the previous operation was successful
  set success mu
  # Whether a synchronous operation is running
  set busy no
  # The description of the last operation's result
  # If disconnected after a connection, this indicates
  # the error message
  set resultMessage {Message should not be shown}

  # The current username
  # Set to $env(USER) when $::PLATFORM=="UNIX", or
  # $env(USERNAME) when $::PLATFORM=="WINDOWS", either
  # prefixed with ~, when logged in locally.
  # Blank if not logged in
  set username {}
  # The current userid
  # Blank for localhost login or not logged in
  set userid {}
  # The password used to log in
  # Blank if not logged in or logged in locally
  set password {}

  # Set to true if a network connection exists to
  # SERVER
  set isConnected no
  # Set to true if a connection exists AND is ready
  # for general commands
  set isReady no

  # The last time we ever heard anything from the server
  set lastReceive {}
  # The last time we sent any message to the server
  set lastSend {}

  # The offset between the server's clock and our own.
  # Network time = [clock seconds] + $::abnet::clockOffset
  set clockOffset 0

  # The filename array maps userid,filename pairs to
  # the fileid for the given file, or 0 for nonexistent.
  # Ex: $::abnet::filename($userid,$filename).
  # This is persistent across sessions.
  # Note that the presence of a mapping does not guarantee
  # it ist still valid.

  # The filestat array records stat information on files,
  # retrieved by statting or opening them.
  # Each entry is an empty list to indicate inability to
  # work with the file,

  # Public control functions which reflect into equivalent
  # internals

  # Returns the network time
  proc nclock {} {
    expr {[clock seconds]+$::abnet::clockOffset}
  }

  # Performs any internal updating necessary.
  proc runproto {} {
    ::abnet::_runproto
    after 5 ::abnet::runproto
  }

  # Returns the number of seconds since a response
  # from the server has been heard (or time spent
  # in busy mode)
  proc timeSinceResponse {} {
    expr {[clock seconds]-$::abnet::lastReceive}
  }

  # Opens a connection to the server and automatically
  # proceeds into secure mode.
  # This call return immediately.
  # busy will immediately become true, as well as isConnected
  # (though a working connection will not yet exist).
  #
  # Preconditions:
  #   !isConnected
  #
  # Once secure mode is entered, isReady becomes true and
  # busy becomes false. success is set to true.
  #
  # On failure, busy and isConnected are both reset to
  # false, and resultMessage is set appropriately. success
  # is set to false.
  #
  # Conditions for failure:
  #   Connection could not be established
  #   Server returned error
  #   Server not understood (general protocol failure)
  #   Timeout
  proc openConnection {} { _openConnection }

  # Kills any current connection, with an optional
  # error message.
  #
  # This will always succeed.
  # isConnected, isReady, and busy will all become false.
  # All login info is reset.
  proc closeConnection {{msg Nonspecific}} {
    _closeConnection $msg
  }

  # Attempts to log in with the given credentials.
  # busy will be come true, until an answer is
  # received.
  #
  # On success, login information is set properly and
  # success is true.
  #
  # On failure, success is false and the returned error
  # message is indicated.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   username is blank
  #
  # Conditions for failure:
  #   Connection failure
  #   Timeout
  #   Invalid credentials
  #   Userid not provided by server
  #   General protocol failure
  #   Other server error
  proc login {username password} { _login $username $password }

  # Attempts to create an account with the given information.
  # Otherwise, same behaviour as login.
  proc createAcct {username password} { _createAcct $username $password }

  # Performs an offline local login.
  # This will set the login information appropriately
  # (though userid will be blank) and set isReady to
  # true, though isConnected will remain false.
  proc offlineLogin {} { _offlineLogin }

  # Terminates the current session, logging out if necessary.
  # This will work for offline logon as well.
  proc logout {} { _logout }

  # Alters the username and/or password of the current account.
  # Busy will be true until an answer is received.
  #
  # On success, username and password are set to match the arguments
  # passed to this function, and success is true.
  #
  # On failure, success is false, the returned error message is
  # indicated, and username and password are unchanged.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  #
  # Conditions for failure:
  #   Connection failure
  #   Timeout
  #   Duplicate username
  #   General protocol failure
  #   Other server error
  proc alterAcct {username password} { _alterAcct $username $password }

  # Deletes the current account and terminates the connection.
  # This always succeeds if the preconditions are met.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  #
  # Besides deleting the account, it has the same effects as logout.
  proc deleteAcct {} { _deleteAcct }

  # Requests the fileid of the given userid, filename
  # pair.
  # Busy will become true until the operation completes.
  # Once completed, the fileid entry will be made in the
  # filenames array, indicating the fileid or zero on
  # failure. success and resultMessage will be set appropriately.
  # If userid is omitted, it defaults to the current userid.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  # --or--
  #   Logged in locally: The command will always fail.
  proc lookupFilename {filename {userid {}}} {
    if {"" == $userid} {
      set userid $::abnet::userid
    }
    _lookupFilename $filename $userid
  }

  # Requests statistics on the given fileid.
  # Busy will become true until the operation completes.
  # Once completed, an entry in the filestat array
  # will be made. This entry will be an empty list on
  # failure, and [size,modified] on success.
  # Success and resultMessage are set appropriately.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  # --or--
  #   Logged in locally: The command will always fail.
  proc stat fileid { _stat $fileid }

  # Requests to download the given file.
  # Busy will become true until the operation completes.
  # Once completed, an entry in the filestat array
  # will be made. This entry will be an empty list on
  # failure, and [size,modified] on success.
  # Success and resultMessage are set appropriately.
  # When successful, data is saved into the given file.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  # --or--
  #   Logged in locally: The command will always fail.
  proc getf {fileid outfile} { _getf $fileid $outfile }

  # Combined lookupFilename/getf
  # It has the exact behaviour of calling lookupFilename
  # if the name is not yet known, then immediately getf
  # after success.
  #
  # This is a compound operation, so success hooks will not work as expected.
  proc getfn {filename outfile {userid {}}} {
    if {"" == $userid} { set userid $::abnet::userid }
    _getfn $filename $outfile $userid
  }

  # Requests to upload the given file.
  # Busy will become true until the operation completes.
  # On success, any updates necessary to the filename and/or
  # filestat arrays are made.
  # After execution, success and resultMessage are set
  # appropriately.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  # --or--
  #   Logged in locally: The command will always fail.
  proc putf {filename infile {public 0}} { _putf $filename $infile $public }

  # Deletes the specified file.
  # Busy will become true until the operation completes.
  # After execution, success and resultMessage are set appropriately.
  #
  # Preconditions:
  #   !busy
  #   isReady
  #   isConnected
  #   userid not blank
  # --or--
  #   Logged in locally: The command will always fail
  proc rmf {fileid} { _rmf $fileid }

  # Requests a user list, with the given ordering, first index, and
  # last exclusive index. If logged in and all parms are valid, this
  # will always succeed. The list will be stored in ::abnet::userlist,
  # as alternating (userid, friendlyName).
  # busy will be true until the list is returned.
  # When logged in locally, results in an empty list.
  proc userls {order first lastex} {
    _userls $order $first $lastex
  }

  # Requests a ship listing for the given userid. If logged in, this will
  # always succeed. The list will be stored in ::abnet::shiplist, as
  # repeating (shipid, fileid, class, shipname).
  # busy will be true until the list is returned.
  # When logged in locally, results in an empty list.
  proc shipls userid {
    _shipls $userid
  }

  # Requests a ship file listing for the current user.
  # The list will be storvd in ::abnet::shipflist as a list of
  # strings; all appropriate ::abnet::filename entries will be made.
  # When logged in locally, this results in an empty list.
  proc shipfls {} {
    _shipfls
  }

  # Marks the given ship as downloaded. This always succeeds (it leaves
  # ::abnet::success et al unchanged).
  proc shipdl shipid {
    _shipdl $shipid
  }

  # Rates the given ship. This always succeeds (it leaves ::abnet::success
  # et al unchanged).
  proc shiprate {shipid positive} {
    _shiprate $shipid $positive
  }

  # Requests the server to return subscription information.
  # On success, the lists are stored in ::abnet::userSubscriptions and
  # ::abnet::shipSubscriptions.
  # ::abnet::busy will be true until the operation completes.
  proc fetchSubscriptions {} {
    _fetchSubscriptions
  }

  # Requests the server to list ships that have changed since the respective
  # LSS dates. When successful, the list will be stored in
  # ::abnet::obsoleteShips. busy will be true until the operation completes.
  proc obsoleteShips {} {
    _obsoleteShips
  }

  # Requests the server to list all ships the user is subscribed to.
  # When successful, the list will be stored in ::abnet::subscribedShips.
  # ::abnet::busy will be true until the operation completes.
  proc subscribedShips {} {
    _subscribedShips
  }

  # Gets information for the given shipid.
  # If successful, the data will be stored in ::abnet::shipinfo($shipid,x)
  # where x is downloads, sumrating, or numrating. If the entries already
  # exist, this proc does not make any request.
  # On completion, success will be set appropriately. Data will only be
  # added on returned data (so a non-existent ship will still result in
  # zeros being added). If not connected, this always fails without setting
  # the array entries.
  proc getShipInfo shipid {
    _getShipInfo $shipid
  }

  # Adds the given user to the subscribed list, if not already in.
  # Does nothing if not connected.
  # Assumes that fetchSubscriptions has already been executed
  # for this connection.
  proc subscribeUser userid {
    _subscribeUser $userid
  }
  # Removes the given user from the subscribed list, if present.
  # Does nothing if not connected.
  # Assumes that fetchSubscriptions has already been executed
  # for this connection.
  proc unsubscribeUser userid {
    _unsubscribeUser $userid
  }
  # Adds the given ship to the subscribed list, if not already in.
  # Does nothing if not connected.
  # Assumes that fetchSubscriptions has already been executed
  # for this connection.
  proc subscribeShip shipid {
    _subscribeShip $shipid
  }
  # Removes the given ship from the subscribed list, if present.
  # Does nothing if not connected.
  # Assumes that fetchSubscriptions has already been executed
  # for this connection.
  proc unsubscribeShip shipid {
    _unsubscribeShip $shipid
  }

  # Submits the specified AI report if ready.
  # Does nothing otherwise.
  proc submitAiReport {spec gen scores ct} {
    _submitAiReport $spec $gen $scores $ct
  }

  # Waits until $busy is false.
  proc sync {} {
    while {$::abnet::busy} {
      after 5
      update
    }
  }

  # Reports to the server that the current job has completed, with the given
  # report data passed along.
  proc jobDone {data} {
    _jobDone $data
  }

  # Reports to the server that the current job has failed, with the given
  # reason.
  proc jobFailed {data} {
    _jobFailed $data
  }

  # Enters "slave mode" for remote jobs.
  proc slaveMode {} {
    set ::abnet::MAXIMUM_MSG_INTERVAL 5
    writeServer make-me-a-slave
  }

  # Requests notification when $::abnet::success is updated.
  # The parameters given will have the new value of success appended (via
  # lappend), and will then be eval'd at global scope.
  # If not currently busy, immediately runs the hook.
  proc successHook {args} {
    _successHook $args
  }

  ### NO DECLARATIONS BELOW THIS POINT SHOULD BE ACCESSED
  ### BY EXTERNAL CODE

  # See whether we should enable the lanpatch.
  # conf.lanpatch MUST exist and be set to true;
  # then, see if
  #   http://192.168.10.197:12544/abendstern/package/manifest
  # results in HTTP/1.1 200 OK. If it does, set
  # ::abnet::SERVER to 192.168.10.197
  #
  # DUE TO THE PREDICTABLE NATURE OF THIS, IT IS A SECURITY
  # RISK! Only those who sometimes live within my LAN should
  # ever enable this.
  catch {
    if {[$ bool conf.lanpatch]} {
      set tok [::http::geturl \
               "http://192.168.10.197:12544/abendstern/package/manifest"\
               -timeout 1000 -validate 1]
      if {"ok" == [::http::status $tok] && "200" == [::http::ncode $tok]} {
        set ::abnet::SERVER 192.168.10.197
      }
      ::http::cleanup $tok
    }
  }

  # The current input/output modes (plain | secure | data)
  # data is only actually valid for inputMode
  set inputMode plain
  set outputMode plain
  # Data read in from input that has yet to be processed
  set inputBuffer {}
  # The current encryption keys for input/output
  set inputKey {}
  set outputKey {}
  set outputsSinceKeyChange 0
  # DHKE intermediate computations
  set dhke_localprivate {}
  set dhke_remotepublic {}
  set dhke_secret {}

  # When inputMode is data, keep track of the number of raw data
  # bytes remaining as well as the proc to call when it completes
  # The callback will take a single argument, which is the raw
  # input data.
  # It will be evaluated with
  #   namespace eval ::abnet $::abnet::inputDataCallback $data
  set inputDataLeft 0
  set inputDataCallback {}

  # The socket to use for IO
  set sock {}

  # The current update function
  set currentUpdate {}

  # The type of action for which we expect an action-status
  set currentAction {}
  # The expected userid,filename pair expected in the next fileid message
  set requestedUfnPair {}

  # The possible messages we expect to get from the server
  set enabledMessages {}

  # Enables the given message patterns
  proc enable args {
    global ::abnet::enabledMessages
    lappend enabledMessages {*}$args
  }

  # Disables the given message patterns
  # No error if they are not present
  proc disable args {
    global ::abnet::enabledMessages
    foreach pattern $args {
      while {-1 != [set ix [lsearch -exact $enabledMessages $pattern]]} {
        set enabledMessages [lreplace $enabledMessages $ix $ix]
      }
    }
  }

  # Alters the given key
  # If the old key is non-empty, it is first deallocated
  # with ::aes::Final
  proc setKey {which key} {
    upvar $which k
    if {[string length $k]} {
      ::aes::Final $k
    }

    set k [::aes::Init cbc [binary format H32 [format %032s $key]] 0123456789ABCDEF]
  }

  # Executes the given message
  proc execMsg {msg args} {
    foreach pattern $::abnet::enabledMessages {
      if {[string match $pattern $msg]} {
        message-$msg {*}$args
        return
      }
    }

    error "Unexpected message: $msg"
  }

  # Executes all messages in the input buffer
  # Each message MUST be terminated with a linefeed
  # Any errors are passed up the stack
  proc execInputMsgs {} {
    while {-1 != [set ix [string first "\n" $::abnet::inputBuffer]]} {
      set msg [string range $::abnet::inputBuffer 0 $ix-1]
      set ::abnet::inputBuffer [string range $::abnet::inputBuffer $ix+1 end]
      set msg [encoding convertfrom utf-8 $msg]
      if {[llength $msg]} {
        execMsg {*}$msg
        if {!$::abnet::busy} {
          set ::abnet::lastReceive [clock seconds]
        }
      }
    }
  }

  # Reads input from the server and executes it
  # if a complete message is received.
  # Updates lastReceive unless busy outside of data mode
  # Any errors are passed up the stack
  proc readServer {} {
    set hasMore yes
    while {$hasMore && [string length $::abnet::sock]} {
      switch $::abnet::inputMode {
        plain {
          set str [gets $::abnet::sock]
          set hasMore [string length $str]
          if {$hasMore} {
            append ::abnet::inputBuffer $str "\n"
            execInputMsgs
          }
        }
        secure {
          set str [read $::abnet::sock $::abnet::BLOCK_SZ]
          set hasMore [string length $str]
          if {$hasMore} {
            set str [::aes::Decrypt $::abnet::inputKey $str]
            append ::abnet::inputBuffer $str
            execInputMsgs
          }
        }
        data {
          set dat [read $::abnet::sock $::abnet::inputDataLeft]
          set hasMore [string length $dat]
          if {$hasMore} {
            set ::abnet::lastReceive [clock seconds]
            incr ::abnet::inputDataLeft -[string length $dat]
            append ::abnet::inputBuffer $dat
            if {0 == $::abnet::inputDataLeft} {
              set ::abnet::inputMode secure
              # We need to nest the inputBuffer in another list so it is
              # not parsed
              eval $::abnet::inputDataCallback [list $::abnet::inputBuffer]
              set ::abnet::inputBuffer {}
            }
          }
        }
      }
    }
  }

  # Writes the given message to the server
  # Updates lastSend
  proc writeServer args {
    set ::abnet::lastSend [clock seconds]
    switch $::abnet::outputMode {
      plain {
        if {[catch {
          puts $::abnet::sock $args
          #flush $::abnet::sock
        } err]} {
          log "Failure writing to server: $err"
          set ::abnet::success no
          set ::abnet::busy no
          set ::abnet::isReady no
          set ::abnet::isConnected no
          set ::abnet::resultMessage $err
          return
        }
      }
      secure {
        incr ::abnet::outputsSinceKeyChange
        if {$::abnet::outputsSinceKeyChange > 1024} {
          set k [crypto_rand]
          set ::abnet::outputsSinceKeyChange 0
          writeServer change-key $k
          setKey ::abnet::outputKey $k
        }

        set str [encoding convertto utf-8 $args]
        append str "\n"
        while {[string length $str] % 16} {
          append str "\n"
        }
        if {[catch {
          puts -nonewline $::abnet::sock [::aes::Encrypt $::abnet::outputKey $str]
          #flush $::abnet::sock
        } err]} {
          log "Failure writing to server: $err"
          set ::abnet::success no
          set ::abnet::busy no
          set ::abnet::isReady no
          set ::abnet::isConnected no
          set ::abnet::resultMessage $err
          return
        }
      }
    }
  }

  proc _runproto {} {
    if {!$::abnet::isConnected} return
    if {[catch {
      readServer
    } err errinfo]} {
      # For debugging
      set debout [open [homeq abneterr.log] w]
      puts $debout "$err\n$errinfo"
      close $debout
      closeConnection $err
      set ::abnet::resultMessage [_ N cxn unexpected]
      return
    }

    if {[string length $::abnet::currentUpdate]} {
      namespace eval ::abnet $::abnet::currentUpdate
    }

    # Check for timeout (but don't time out for SYSTEM)
    if {$::abnet::busy &&
        $::abnet::userid != 0 &&
        $::abnet::MAXIMUM_BUSY_TIME < [clock seconds]-$::abnet::lastReceive} {
      closeConnection "Timed out"
      set ::abnet::success no
      set ::abnet::resultMessage [_ N cxn timeout]
    } elseif {!$::abnet::busy
          && $::abnet::MAXIMUM_MSG_INTERVAL < [clock seconds]-$::abnet::lastSend} {
      writeServer ping
    }
  }

  proc _openConnection {} {
    set ::abnet::isConnected yes
    set ::abnet::busy yes
    set ::abnet::inputMode plain
    set ::abnet::outputMode plain
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::inputDataLeft 0
    set ::abnet::inputBuffer {}
    set ::abnet::enabledMessages [list error abendstern clock-sync job]

    if {[catch {
      set ::abnet::sock [socket -async $::abnet::SERVER $::abnet::PORT]
      fconfigure $::abnet::sock -blocking 0 -buffering none
      writeServer abendstern $::abnet::NETWVERS $::abnet::ABVERS
    }]} {
      # Immediate failure
      set ::abnet::isConnected no
      set ::abnet::busy no
      set ::abnet::success no
      set ::abnet::resultMessage [_ N cxn rejected]
      return
    }

    set ::abnet::currentUpdate {}
  }

  # Destroys all current login information and resets
  # to the not-logged-in state
  proc endSession {} {
    set ::abnet::username {}
    set ::abnet::userid {}
    set ::abnet::password {}
  }

  # Terminates the current connection after running
  # endSession, then resets variables to a not-connected
  # state
  proc endConnection {} {
    sync
    endSession
    # Delay so any pending message can be transmitted
    after 512 close $::abnet::sock
    set ::abnet::sock {}
    set ::abnet::isConnected no
    set ::abnet::isReady no
    set ::abnet::currentUpdate {}
    set ::abnet::busy no
  }
  proc _closeConnection msg {
    if {[catch { writeServer error {} $msg } err errinfo]} {
      log "$err\n$errinfo"
    }

    endConnection
  }

  proc _login {username password} {
    sync
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::username $username
    set ::abnet::password $password
    set ::abnet::currentAction login

    writeServer pre-account-login $username $password
    enable action-status user-id
  }

  proc _createAcct {username password} {
    sync
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::username $username
    set ::abnet::password $password
    set ::abnet::currentAction login ;# Same properties as login, even though creation

    writeServer pre-account-create $username $password
    enable action-status user-id
  }

  proc _alterAcct {username password} {
    sync
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::alteredUsername $username
    set ::abnet::alteredPassword $password
    set ::abnet::currentAction alter

    writeServer top-account-rename $username $password
    enable action-status
  }

  proc _deleteAcct {} {
    sync
    writeServer top-account-delete
    endConnection
  }

  proc _offlineLogin {} {
    set ::abnet::clockOffset 0
    if {$::abnet::isConnected} {
      endConnection
    }
    set ::abnet::isConnected no
    set ::abnet::isReady yes
    switch -exact -- $::PLATFORM {
      UNIX      { set ::abnet::username ~$::env(USER) }
      WINDOWS   { set ::abnet::username ~$::env(USERNAME) }
      default   { error "Unsupported platform $::PLATFORM" }
    }
    set ::abnet::userid {}
    set ::abnet::password {}
    set ::abnet::success yes
  }

  proc _logout {} {
    sync
    if {$::abnet::userid != ""} {
      writeServer [list top-account-logout]
    }
    if {$::abnet::isConnected} {
      endConnection
    } else {
      # Local login
      set ::abnet::isReady no
    }
  }

  proc _lookupFilename {filename userid {getfnext no}} {
    sync
    set ::abnet::requestedUfnPair "$userid,$filename"
    if {!$::abnet::isConnected} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N general failure]
      set ::abnet::filenames($userid,$filename) 0
      return
    }

    writeServer top-file-lookup $userid $filename
    set ::abnet::busy yes
    set ::abnet::currentAction lookupFilename-$getfnext
    enable fileid
    # Give 30 seconds for file transfers
    set ::abnet::lastReceive [expr {[clock seconds]+20}]
  }

  proc _stat fileid {
    if {!$::abnet::isConnected} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N general failure]
      set ::abnet::filestat($fileid) {}
      return
    }

    sync
    writeServer top-file-stat $fileid
    set ::abnet::busy yes
    set ::abnet::currentAction stat
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::requestedUfnPair $fileid
    enable file-info
  }

  proc _getf {fileid outfile} {
    if {!$::abnet::isConnected} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N general failure]
      set ::abnet::filestat($fileid) {}
      return
    }

    sync
    writeServer top-file-open $fileid
    set ::abnet::busy yes
    set ::abnet::currentAction open
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::requestedUfnPair $fileid
    enable file-info
    set ::abnet::inputDataCallback [list ::abnet::getf-receive $outfile]
    # Don't enter data mode until we get the file-info
  }

  proc _getfn {filename outfile userid} {
    if {![info exists ::abnet::filenames($userid,$filename)]} {
      set ::abnet::getfn_outfile $outfile
      _lookupFilename $filename $userid yes
    } elseif {0 == $::abnet::filenames($userid,$filename)} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N file not_found]
    } else {
      _getf $::abnet::filenames($userid,$filename) $outfile
    }
  }

  proc _putf {filename infile public} {
    if {!$::abnet::isConnected} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N general failure]
      return
    }

    sync
    writeServer top-post-file $filename [file size $infile] $public
    # Write the data
    # Since sock is async, this will complete as soon as all data is
    # read from the file.
    set in [open $infile rb]
    fcopy $in $::abnet::sock
    close $in

    set ::abnet::busy yes
    set ::abnet::currentAction putf
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::putf_filename $filename
    set ::abnet::putf_size [file size $infile]
    enable action-status
  }

  proc _rmf fileid {
    if {"" == $::abnet::userid} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N general failure]
      return
    }

    sync
    writeServer top-file-delete $fileid
    set ::abnet::filestat($fileid) {}

    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::currentAction generic
    enable action-status
  }

  proc _userls {order first last} {
    if {"" == $::abnet::userid} {
      set ::abnet::success yes
      set ::abnet::userlist {}
      return
    }

    sync
    enable user-list
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    writeServer top-user-ls $order $first $last
  }

  proc _shipls userid {
    if {"" == $::abnet::userid} {
      set ::abnet::success yes
      set ::abnet::shiplist {}
      return
    }

    sync
    enable ship-list
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    writeServer top-ship-ls $userid
  }

  proc _shipfls {} {
    if {"" == $::abnet::userid} {
      set ::abnet::success yes
      set ::abnet::shipflist {}
      return
    }

    sync
    enable ship-file-list
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    writeServer top-ship-file-ls
  }

  proc _shipdl shipid {
    if {"" == $::abnet::userid} return
    sync
    writeServer top-record-ship-download $shipid
  }

  proc _shiprate {shipid positive} {
    if {"" == $::abnet::userid} return
    sync
    writeServer top-rate-ship $shipid $positive
  }

  proc _fetchSubscriptions {} {
    if {"" == $::abnet::userid} {
      set ::abnet::success no
      set ::abnet::shipSubscriptions {}
      set ::abnet::userSubscriptions {}
      return
    }

    sync
    writeServer top-get-subscriber-info
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    enable subscriber-info
  }

  proc _obsoleteShips {} {
    if {"" == $::abnet::userid} {
      set ::abnet::success no
      set ::abnet::obsoleteShips {}
      return
    }

    sync
    writeServer top-ships-obsolete [lssi_lastUpload] [lssi_lastDownload]
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    enable ships-obsolete
  }

  proc _subscribedShips {} {
    if {"" == $::abnet::userid} {
      set ::abnet::success no
      set ::abnet::subscribedShips {}
      return
    }

    sync
    writeServer top-ships-all
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    enable ship-list-all
  }

  proc _getShipInfo shipid {
    if {[info exists ::abnet::shipinfo($shipid,downloads)]} {
      set ::abnet::success yes
      return
    }
    if {!$::abnet::isConnected} {
      set ::abnet::success no
      return
    }

    sync
    writeServer top-ship-info $shipid
    set ::abnet::busy yes
    set ::abnet::lastReceive [clock seconds]
    set ::abnet::shipinfoshipid $shipid
    enable ship-info
  }

  proc _subscribeUser userid {
    if {!$::abnet::isConnected} return
    set ix [lsearch -exact $::abnet::userSubscriptions $userid]
    if {$ix == -1} {
      lappend ::abnet::userSubscriptions $userid
      sync
      writeServer top-subscribe-user $userid
    }
  }
  proc _unsubscribeUser userid {
    if {!$::abnet::isConnected} return
    set ix [lsearch -exact $::abnet::userSubscriptions $userid]
    if {$ix != -1} {
      set ::abnet::userSubscriptions [lreplace $::abnet::userSubscriptions $ix $ix]
      sync
      writeServer top-unsubscribe-user $userid
    }
  }
  proc _subscribeShip shipid {
    if {!$::abnet::isConnected} return
    set ix [lsearch -exact $::abnet::shipSubscriptions $shipid]
    if {$ix == -1} {
      lappend ::abnet::shipSubscriptions $shipid
      sync
      writeServer top-subscribe-ship $shipid
    }
  }
  proc _unsubscribeShip shipid {
    if {!$::abnet::isConnected} return
    set ix [lsearch -exact $::abnet::shipSubscriptions $shipid]
    if {$ix == -1} {
      set ::abnet::shipSubscriptions [lreplace $::abnet::shipSubscriptions $ix $ix]
      sync
      writeServer top-unsubscribe-ship $shipid
    }
  }

  proc _submitAiReport {spec gen scores ct} {
    if {!$::abnet::isConnected
    ||  !$::abnet::isReady
    ||  $::abnet::busy} return
    set cortex 0
    foreach {instance score} $scores {
      writeServer top-ai-report-2 $spec $gen $cortex $instance $score $ct
      set ct 0
      incr cortex
    }
  }

  proc _jobDone {data} {
    sync
    log "Job done: $data"
    writeServer job-done {*}$data
    enable job
  }

  proc _jobFailed {why} {
    sync
    log "Job failed: $why"
    writeServer job-failed $why
    enable job
  }

  proc _successHook {code} {
    trace add variable ::abnet::success write \
        [list ::abnet::successHookTrigger $code]
    if {!$::abnet::busy} {
      set ::abnet::success $::abnet::success
    }
  }

  proc successHookTrigger {code args} {
    # Remove the trace since this is only supposed to run once
    trace remove variable ::abnet::success write \
        [list ::abnet::successHookTrigger $code]
    # Add the current value of success and run
    lappend code $::abnet::success
    namespace eval :: $code
  }

  proc message-error {l10n english} {
    set report $english
    catch {
      set m [_ N {*}$l10n]
      if {$m != "####" && $m != "#####"} {
        set report $m
      }
    }

    endConnection
    set ::abnet::success no
    set ::abnet::resultMessage $report
    log "Disconnecting due to server error: $report ($english)"
  }

  proc message-abendstern vers {
    disable abendstern
    enable dhke-second

    set ::abnet::dhke_localprivate [crypto_rand]
    writeServer dhke-first [crypto_powm $::abnet::DHKE_BASE $::abnet::dhke_localprivate]
  }

  proc message-clock-sync seconds {
    set local [clock seconds]
    log "Client time: [clock format $local   -format {%Y.%m.%d %T %Z}]"
    log "Server time: [clock format $seconds -format {%Y.%m.%d %T %Z}]"
    set ::abnet::clockOffset [expr {$seconds-$local}]
    log "Offset: $::abnet::clockOffset seconds"
  }

  proc message-dhke-second remotepublic {
    set ::abnet::dhke_remotepublic $remotepublic
    disable dhke-second
    set ::abnet::dhke_secret [crypto_powm $remotepublic $::abnet::dhke_localprivate]
    writeServer begin-secure

    # Enter secure mode
    set key [string range [format "%032s" $::abnet::dhke_secret] end-31 end]
    setKey ::abnet::inputKey $key
    setKey ::abnet::outputKey $key
    set ::abnet::inputMode secure
    set ::abnet::outputMode secure
    enable change-key
    fconfigure $::abnet::sock -buffering none -translation binary

    # Done setting connection up
    set ::abnet::busy no
    set ::abnet::isReady yes
    set ::abnet::success yes
    set ::abnet::resultMessage [_ N general success]
    set ::abnet::lastReceive [clock seconds]
  }

  proc message-change-key k {
    setKey ::abnet::inputKey $k
  }

  proc message-action-status {success l10n english} {
    set ::abnet::busy no
    set ::abnet::success $success
    disable action-status

    set result $english
    catch {
      set msg [_ N {*}$l10n]
      if {$msg != "####" && $msg != "#####"} {
        set result $msg
      }
    }

    set ::abnet::resultMessage $result

    aux-action-status-$::abnet::currentAction $success
  }

  proc aux-action-status-generic success {}

  proc aux-action-status-login success {
    disable user-id
    if {$success} {
      if {![string length $::abnet::userid]} {
        closeConnection "Userid not provided"
        set ::abnet::resultMessage [_ N protocol error]
      }
    } else {
      endSession ;# Remove partial login info
    }
  }

  proc aux-action-status-alter success {
    if {$success} {
      # Rename/password alteration successful, so set
      # the new values
      set ::abnet::username $::abnet::alteredUsername
      set ::abnet::password $::abnet::alteredPassword
    }
  }

  proc message-user-id id {
    disable user-id
    if {![string is integer $id]} {
      disable action-status
      closeConnection "Bad userid"
      set ::abnet::success no
      set ::abnet::resultMessage [_ N protocol error]
    } else {
      set ::abnet::userid $id
    }
  }

  proc message-fileid {userid filename fileid} {
    if {$::abnet::requestedUfnPair != "$userid,$filename"} {
      closeConnection "Unexpected fileid return"
      set ::abnet::success no
      set ::abnet::resultMessage [_ N protocol error]
      return
    }
    set ::abnet::busy no
    if {"" == $fileid} {
      set ::abnet::success no
      set ::abnet::resultMessage [_ N file not_found]
      set ::abnet::filenames($userid,$filename) 0
    } else {
      set ::abnet::success yes
      set ::abnet::resultMessage [_ N general success]
      set ::abnet::filenames($userid,$filename) $fileid

      if {$::abnet::currentAction == "lookupFilename-yes"} {
        _getf $fileid $::abnet::getfn_outfile
      }
    }
    disable fileid
  }

  proc message-file-info {fileid size modified} {
    disable file-info
    if {$fileid != $::abnet::requestedUfnPair
    &&  {} != $::abnet::requestedUfnPair} {
      closeConnection "file-info on different file ($fileid $::abnet::requestedUfnPair)"
      set ::abnet::success no
      set ::abnet::resultMessage [_ N protocol error]
      return
    }

    if {$size != "" && $modified != ""} {
      # Success
      set ::abnet::filestat($fileid) [list $size $modified]

      if {$::abnet::currentAction == "stat"} {
        # Done
        set ::abnet::busy no
        set ::abnet::success yes
        set ::abnet::resultMessage [_ N general success]
      } else {
        # Ready for data
        set ::abnet::lastReceive [clock seconds]
        set ::abnet::inputMode data
        set ::abnet::inputDataLeft $size
      }
    } else {
      # Failure
      set ::abnet::busy no
      set ::abnet::success no
      set ::abnet::resultMessage [_ N general failure]
      set ::abnet::filestat($fileid) {}
    }
  }

  proc getf-receive {outfile data} {
    set out [open $outfile wb]
    puts -nonewline $out $data
    close $out

    set ::abnet::busy no
    set ::abnet::success yes
    set ::abnet::resultMessage [_ N general success]
  }

  proc aux-action-status-putf success {
    if {$success} {
      # success holds the fileid.
      # Update filenames and filestat
      set ::abnet::filenames($::abnet::userid,$::abnet::putf_filename) $success
      set ::abnet::filestat($success) [list $::abnet::putf_size [clock seconds]]
    }
  }

  proc message-user-list args {
    disable user-list
    set ::abnet::success yes
    set ::abnet::busy no
    set ::abnet::userlist $args
  }
  proc message-ship-list args {
    disable ship-list
    set ::abnet::success yes
    set ::abnet::busy no
    set ::abnet::shiplist $args
  }
  proc message-ship-file-list args {
    disable ship-file-list
    set ::abnet::success yes
    set ::abnet::busy no
    set ::abnet::shipflist $args

    foreach {fileid filename} $args {
      set ::abnet::filenames($::abnet::userid,$filename) $fileid
    }
  }

  proc message-subscriber-info {us ss} {
    set ::abnet::busy no
    set ::abnet::success yes
    set ::abnet::userSubscriptions $us
    set ::abnet::shipSubscriptions $ss
    disable subscriber-info
  }

  proc message-ships-obsolete args {
    set ::abnet::busy no
    set ::abnet::success yes
    set ::abnet::obsoleteShips $args
    disable ships-obsolete
  }

  proc message-ship-list-all args {
    set ::abnet::busy no
    set ::abnet::success yes
    set ::abnet::subscribedShips $args
    disable ship-list-all
  }

  proc message-ship-info {downloads sumrating numrating} {
    set ::abnet::shipinfo($::abnet::shipinfoshipid,downloads) $downloads
    set ::abnet::shipinfo($::abnet::shipinfoshipid,sumrating) $sumrating
    set ::abnet::shipinfo($::abnet::shipinfoshipid,numrating) $numrating
    set ::abnet::busy no
    set ::abnet::success yes
  }

  proc message-job {type args} {
    disable job
    if {[catch {
      create-job-$type {*}$args
    } err]} {
      jobFailed "Error creating job: $type $args: $err"
    }
  }

  after idle ::abnet::runproto
}
