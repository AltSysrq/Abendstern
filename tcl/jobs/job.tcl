# A Job is a task assigned by the Abendstern Network server which is executed
# progressively in the background.
#
# Each job type must create a proc of the form
#   proc create-job-$type {...} { ... }
# where $type is the network-named type for the job, and the arguments are
# whatever it should expect to receive via the job message.
#
# Note: Subclasses MUST be prepared to be destroyed AT ANY TIME. If the network
# connection is lost, the Job will delete itself to avoid colliding with a
# later, new connection.
# Additionally, the destructor is NOT guaranteed to be called before the
# program shuts down. Thus, the subclass should avoid externally-visible
# operations which have effects that persist beyond one frame. For example,
# when downloading a libconfig file, the data should be written to a file, the
# Config loaded, and the file deleted immediately thereafter, so that, if the
# program exits, no temporary file is left.
class Job {
  # Determines the current "stage of processing" for this Job. For each stage,
  # the subclass must define methods "$currentStage-exec" and
  # "$currentStage-interval". -exec is called (without arguments) to execute
  # the current part of the job. -interval is called (without arguments) and
  # returns the (minimum) number of milliseconds that should elapse before the
  # next call to -exec.
  protected variable currentStage
  variable alive yes

  # Constructs the Job, starting in the given stage
  constructor {initStage} {
    set currentStage $initStage
    after idle [list $this update]
  }

  # Indicates that the Job is done. The given arguments (if any) are sent to
  # the server in the job-done message. This Job will be deleted on the next
  # update.
  method done {args} {
    ::abnet::jobDone $args
    set alive no
    after idle [list delete object $this]
  }

  # Indicates that the Job has failed for the given reason. The failure message
  # is sent to the server, and this Job will be deleted on the next update.
  method fail {why} {
    ::abnet::jobFailed $why
    set alive no
    after idle [list delete object $this]
  }

  # Callback for whenever the after script triggers
  method update {} {
    # Die if the network connection is gone
    if {!$::abnet::isReady || $::abnet::userid == {}} {
      after idle [list delete object $this]
      set alive no
      return
    }

    if {[catch {
      $this $currentStage-exec
    } err info]} {
      log "Job failed due to unexpected error: $err\n$info"
      fail "Unexpected error: $err"
      return
    }
    if {$alive} {
      after [$this $currentStage-interval] [list $this update]
    }
  }

  method fqn {} { return $this }
}

# Creates a pair of methods which fetch a file and store it, then change to
# another state.
# Fileid and filename are variable names which must be accessible to the
# methods.
# Postexec is code to run before changing stage but after the file has been
# retrieved; it is spliced into the method.
#
# The entry state is called start-fetch-KEY
proc job-fetch-files-state {key fileid filename nextStage {postexec {}}} {
  uplevel 1 [string map [list KEY $key FILEID $fileid \
                             FILENAME $filename NXT $nextStage PEX $postexec] \
                 {
                   variable fetch-KEY-success mu
                   method start-fetch-KEY-exec {} {
                     if {!$::abnet::busy} {
                       ::abnet::getf $FILEID $FILENAME
                       set currentStage wait-for-fetch-KEY
                       ::abnet::successHook $this fetch-KEY-hook
                     }
                   }

                   method fetch-KEY-hook {suc} {
                     set fetch-KEY-success $suc
                   }

                   method start-fetch-KEY-interval {} {
                     return 100
                   }

                   method wait-for-fetch-KEY-exec {} {
                     if {${fetch-KEY-success} ne {mu}} {
                       if {${fetch-KEY-success}} {
                         # Fetched successfully
                         PEX
                         set currentStage NXT
                       } else {
                         # Couldn't download the file
                         fail "Couldn't download $FILEID"
                       }
                     }
                   }

                   method wait-for-fetch-KEY-interval {} {
                     return 1000
                   }
                 }]
}

# Returns a string usable as a postexec in job-fetch-files-state which loads
# a file into a config.
# Filename is a variable reachable in the method, but mount is substituted
# verbatim.
proc job-pex-mountconf {filename mount} {
  string map [list FILENAME $filename MOUNT $mount] {
    if {[catch {
      $ open $FILENAME MOUNT
    } err]} {
      fail $err
    }
  }
}

source tcl/jobs/render_ship_job.tcl
source tcl/jobs/ship_match_job.tcl