# Background job for ship matches
class ShipMatchJob {
  inherit Job

  variable testFileId
  variable testCount
  variable againstFileId
  variable againstCount
  variable vm
  variable tempt
  variable tempa

  constructor {stt sttc sa sac} {
    Job::constructor start-fetch-test
  } {
    set testFileId $stt
    set testCount $sttc
    set againstFileId $sa
    set againstCount $sac

    set tempt [tmpname shipmatch]
    set tempa [tmpname shipmatch]
    set vm 0
  }

  destructor {
    del $vm
    catch { $ close ship_match_job_test }
    catch { $ close ship_match_job_against }
    file delete $tempt $tempa
  }

  job-fetch-files-state test testFileId tempt start-fetch-against \
      [job-pex-mountconf tempt ship_match_job_test]
  job-fetch-files-state against againstFileId tempa load-vm \
      [job-pex-mountconf tempa ship_match_job_against]

  method load-vm-exec {} {
    if {[catch {
      set vm [VersusMatch_create ship_match_job_test $testCount \
                  ship_match_job_against $againstCount]

      set currentStage step
    } err]} {
      fail $err
    }
  }

  method load-vm-interval {} {
    return 1000
  }

  method step-exec {} {
    if {![$vm step]} {
      # Done
      done [$vm score]
    }
  }

  method step-interval {} {
    # Run at up to 200% realtime
    return 10
  }
}

proc create-job-ship-match {stt sttc sa sac} {
  if {![string is integer -strict $sttc] ||
      ![string is integer -strict $sac ] ||
      $sttc <= 0 || $sttc > 5 ||
      $sac  <= 0 || $sac  > 5} {
    error "Bad integer"
  }

  new ShipMatchJob $stt $sttc $sa $sac
}
