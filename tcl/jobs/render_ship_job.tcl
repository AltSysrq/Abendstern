# Background job for ship rendering
class ShipRenderJob {
  inherit Job

  variable fileId
  variable field 0
  variable ship 0
  variable sir 0
  variable tempShip
  variable renderOutput

  constructor {fid} {
    Job::constructor start-fetch
  } {
    set fileId $fid
    set field [new GameField default 1 1]
    set tempShip [tmpname render]
    set renderOutput [tmpname render]
  }

  destructor {
    del $sir $ship $field
    catch {
      $ close render_ship_job
    }
    file delete $tempShip
    file delete $renderOutput
  }

  method start-fetch-exec {} {
    if {!$::abnet::busy} {
      ::abnet::getf $fileId $tempShip
      set currentStage wait-for-fetch
    }
  }

  method start-fetch-interval {} {
    return 100
  }

  method wait-for-fetch-exec {} {
    if {!$::abnet::busy} {
      if {[llength $::abnet::filestat($fileId)]} {
        # Fetched successfully
        # Try to mount
        if {[catch {
          $ open $tempShip render_ship_job
        } err]} {
          fail $err
          return
        }

        # OK, load the ship next
        set currentStage load-ship
      } else {
        # Couldn't download ship
        fail "Couldn't download $fileId"
      }
    }
  }

  method wait-for-fetch-interval {} {
    return 1000
  }

  method load-ship-exec {} {
    if {[catch {
      set ship [loadShip $field render_ship_job]
    } err]} {
      fail "Couldn't load ship: $err"
      return
    }

    set sir [ShipImageRenderer_create $ship]
    if {$sir == 0} {
      fail "Ship image rendering not supported."
      return
    }

    set currentStage render-ship
  }

  method load-ship-interval {} {
    return 1000
  }

  method render-ship-exec {} {
    if {![$sir renderNext]} {
      # Done rendering, or failed
      # (We won't find out until saving)
      set currentStage save-image
    }
  }

  method render-ship-interval {} {
    return 64
  }

  method save-image-exec {} {
    if {[$sir save $renderOutput]} {
      # Success, upload to server
      set currentStage upload-result
    } else {
      # Failed
      fail "Could not render ship image"
    }
  }

  method save-image-interval {} {
    return 1000
  }

  method upload-result-exec {} {
    if {!$::abnet::busy} {
      ::abnet::putf ".tmp.render.$fileId" $renderOutput
      set currentStage wait-for-upload
    }
  }

  method upload-result-interval {} {
    return 1000
  }

  method wait-for-upload-exec {} {
    if {!$::abnet::busy} {
      # Upload is done, see if it succeeded
      # (We can't check $::abnet::success, because something may have happened
      # while we were asleep)
      if {[info exists ::abnet::filenames($::abnet::userid,$renderOutput)]} {
        # Success
        done $::abnet::filenames($::abnet::userid,$renderOutput)
      } else {
        # Failed to upload
        fail "Couldn't upload renderOutput"
      }
    }
  }

  method wait-for-upload-interval {} {
    return 1000
  }
}

proc create-job-render-ship {fileid} {
  if {![string is integer -strict $fileid]} {
    error "Bad fileid: $fileid"
  }

  new ShipRenderJob $fileid
}
