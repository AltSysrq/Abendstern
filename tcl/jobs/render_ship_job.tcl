# Background job for ship rendering
class ShipRenderJob {
  inherit Job

  variable fileId
  variable field 0
  variable ship 0
  variable sir 0
  variable tempShip
  variable renderOutput
  variable uploadName

  constructor {fid} {
    Job::constructor start-fetch-ship
  } {
    set fileId $fid
    set field [new GameField default 1 1]
    set tempShip [tmpname render]
    set renderOutput [tmpname render]
    set uploadName ".tmp.render.$fid"
  }

  destructor {
    del $sir $ship $field
    catch {
      $ close render_ship_job
    }
    file delete $tempShip
    file delete $renderOutput
  }

  job-fetch-files-state ship fileId tempShip load-ship \
      [job-pex-mountconf tempShip render_ship_job]

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
      ::abnet::putf $uploadName $renderOutput
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
      if {[info exists ::abnet::filenames($::abnet::userid,$uploadName)]} {
        # Success
        done $::abnet::filenames($::abnet::userid,$uploadName)
        # Gets deleted implicitly
        unset ::abnet::filenames($::abnet::userid,$uploadName)
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
