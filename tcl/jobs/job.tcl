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

  # Constructs the Job, starting in the given stage
  constructor {initStage} {
    set currentStage initStage
    after idle [list $this update]
  }

  # Indicates that the Job is done. The given arguments (if any) are sent to
  # the server in the job-done message. This Job will be deleted on the next
  # update.
  method done {args} {
    ::abnet::jobDone $args
    after idle [list delete object $this]
  }

  # Indicates that the Job has failed for the given reason. The failure message
  # is sent to the server, and this Job will be deleted on the next update.
  method fail {why} {
    ::abnet::jobFailed $why
    after idle [list delete object $this]
  }

  # Callback for whenever the after script triggers
  method update {} {
    # Die if the network connection is gone
    if {!$::abnet::isReady || $::abnet::userid == {}} {
      after idle [list delete object $this]
      return
    }

    $this $currentStage-exec
    after [$this $currentStage-interval] [list $this update]
  }
}
