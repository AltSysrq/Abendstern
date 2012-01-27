package require http
package require Itcl
package require gui
namespace import ::itcl::*

set IP_SUBNETS8  {192.168.10 67.162.144 67.162.145 67.165.206 67.165.207}
set IP_ADDRESSES {abendstern.servegame.com 192.168.10.197}

# The SelfUpdater is a helper Mode that allows Abendstern
# to download and install its own updates from my server.
# A manifest is downloaded on startup that lists MD5 sums
# and files, from
#   http://host:12544/abendstern/package/manifest
# It queues for downloading and updating any file that is
# either not on record (as stored in manifest.dat)
# or has a different MD5 sum.
# Most files are stored in a temporary file after being
# downloaded. When all downloading is finished, each is
# copied on top of the local version (after deleting the
# local version first).
# Files ending in .exe or .dll are treated specially.
# They are downloaded into a temporary directory "deferred"
# with their intended names. When the program exits,
# "apply_update.bat" is invoked, which copies them on top
# of the old versions, then deletes them and removes the
# directory.
# abendstern.rc is also treated specially. It is not copied
# onto the old version. Instead, the temporary file is
# mounted and recursively searched. Any settings that don't
# exist in the current version are copied from the new version.

class SelfUpdater {
  inherit ::gui::Mode

  variable ltitle
  variable laction
  variable lfile
  variable pfile
  variable ptotal
  variable bclose
  variable mainpanel

  #srcfile list
  variable downloadQueue
  #tmpname truname
  variable filesToMove
  # List of files to delete on exit
  variable temporaryFiles
  # If true, we need to run apply_update.bat when we
  # exit
  variable needRunApplyUpdate
  # If true, we need to merge the new and old abendstern.rc
  # files
  variable needMergeConfigs

  # Set to no at first, and only copied from the respective
  # above AFTER successful application
  variable reallyNeedRunApplyUpdate
  variable reallyNeedMergeConfigs

  # Dict mapping filenames to MD5 sums
  variable localManifest

  # Store the working remote host here once we successfully
  # download and parse the manifest
  variable server

  # Queue of hosts to try
  variable hostQueue

  # For calculating total progress
  variable currProgress
  variable maxProgress

  # Set to true while applying updates
  variable isApplying

  constructor {} {
    ::gui::Mode::constructor
  } {
    set reallyNeedMergeConfigs no
    set reallyNeedRunApplyUpdate no

    set ltitle [new ::gui::Label "Abendstern Self-Update" centre]
    set laction [new ::gui::Label "Searching for server..." left]
    set lfile [new ::gui::Label "???" left]
    set pfile [new ::gui::ProgressBar]
    set ptotal [new ::gui::ProgressBar]
    set bclose [new ::gui::Button "Abort" "$this abort"]
    $bclose setCancel

    set mainpanel [new ::gui::BorderContainer 0 0.01]
    set main [new ::gui::VerticalContainer 0.01]
    $main add $laction
    $main add $lfile
    $main add $pfile
    $main add $ptotal
    $mainpanel setElt top $ltitle
    $mainpanel setElt centre $main
    $mainpanel setElt bottom $bclose
    set root [new ::gui::ComfyContainer [new ::gui::Frame $mainpanel]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0

    set hostQueue {}
    foreach ipa $::IP_ADDRESSES {
      lappend hostQueue $ipa
    }
    foreach subnet $::IP_SUBNETS8 {
      for {set i 2} {$i < 255} {incr i} {
        lappend hostQueue $subnet.$i
      }
    }

    set currProgress 0
    set maxProgress [llength $hostQueue]

    if {[file exists "manifest"]} {
      # Read the local manifest in
      set f [open "manifest" r]
      set man [read $f]
      close $f
      set localManifest {}
      foreach {md5 filename} $man {
        dict set localManifest $filename $md5
      }
    } else {
      # Empty local manifest
      set localManifest {}
    }

    set downloadQueue {}
    set temporaryFiles {}
    set needRunApplyUpdate no
    set needMergeConfigs no
    set isApplying no

    tryContactNextServer
  }

  method draw {} {
    if {$isApplying} applyNext
    ::update
    ::gui::Mode::draw
  }

  method tryContactNextServer {} {
    if {[string length $hostQueue]} {
      set host [lindex $hostQueue 0]
      set hostQueue [lrange $hostQueue 1 end]
      $lfile setText $host
      $pfile setProgress 0
      $ptotal setProgress [expr {$currProgress*1.0/$maxProgress}]
      incr currProgress

      if {[catch {
        ::http::geturl "http://$host:12544/abendstern/package/manifest" \
          -command  "$this receiveManifest $host" \
          -progress "$this progress" -timeout 10240
      }]} {
        after 250 "$this tryContactNextServer"
      }
    } else {
      displayMessage "\a\[(danger)Error\a\]" [string map [list "`" "\n"] [concat \
        "Self-Update was not able to contact any server. Most likely,"\
        "this means that:"\
        "`+ Your network or Internet access is non-functional."\
        "`+ The Self-Updater no longer functions properly."\
        "`+ Power is down at my appartment, or"\
        "`+ ComCast screwed something up again."\
        "`Please try again later."]]
    }
  }

  method progress {tok tot curr} {
    if {$tot != 0} {
      $pfile setProgress [expr {$curr*1.0/$tot}]
    } else {
      $pfile setProgress 0
    }
  }

  method receiveManifest {host tok} {
    if {[::http::status $tok] == "ok"
    &&  [string match 2?? [::http::ncode $tok]]} {
      # Success!
      set server $host
      set downloadQueue {}
      # Read the manifest now
      foreach {md5 filename} [::http::data $tok] {
        if {![dict exists $localManifest $filename]
        ||  [dict get $localManifest $filename] != $md5} {
          # Need to download
          lappend downloadQueue $filename

          # Update local manifest
          dict set localManifest $filename $md5
        }
      }

      set currProgress 0
      set maxProgress [llength $downloadQueue]

      # If nothing to download, tell the user so
      if {![llength $downloadQueue]} {
        displayMessage "Already Up-to-Date" [string map [list "`" "\n"] [concat \
          "According to the server, no files have changed between the" \
          "version of Abendstern you already have and the version stored" \
          "on the server. No action has been taken."\
          "`Abendstern will exit when you press Close."]]
      } else {
        $laction setText "Downloading updates..."
        downloadNext
      }
    } else {
      # Try next
      after 250 "$this tryContactNextServer"
    }

    ::http::cleanup $tok
  }

  method downloadNext {} {
    if {![llength $downloadQueue]} {
      if {[catch {
        set isApplying yes
        $laction setText "Applying updates..."
        $pfile setProgress 1
        $ptotal setProgress 0
        set currProgress 0
        set maxProgress [llength $filesToMove]
      } err]} {
        puts "downloadNext final error"
        puts $err
        puts $::errorInfo
        exit
      }
      return
    }

    $pfile setProgress 0
    $ptotal setProgress [expr {$currProgress*1.0/$maxProgress}]
    incr currProgress

    set filename [lindex $downloadQueue 0]
    set downloadQueue [lrange $downloadQueue 1 end]

    $lfile setText $filename

    ::http::geturl "http://$server:12544/abendstern/package/$filename" \
      -command  "$this receive $filename" \
      -progress "$this progress"
  }

  method receive {filename tok} {
    if {[::http::status $tok] != "ok"
    ||  ![string match 2?? [::http::ncode $tok]]} {
      displayMessage "\a\[(danger)Error\a\]" [string map [list "`" "\n"] [concat \
        "The file $filename could not be downloaded. The server returned:"\
        "`[::http::code $tok]"\
        "This installation of Abendstern has not been modified."\
        "Click Close to exit the application."]]
      ::http::cleanup $tok
      return
    }

    switch -glob -- $filename {
      abendstern.rc {
        set needMergeConfigs yes
        lappend temporaryFiles "abendstern.rc.new"
        set f [open "abendstern.rc.new" w]
        puts $f [::http::data $tok]
        close $f
      }
      *.exe -
      *.dll {
        set needRunApplyUpdate yes
        file mkdir deferred
        set basename [file tail $filename]
        set f [open "deferred/$basename" wb]
        puts -nonewline $f [::http::data $tok]
        close $f
      }
      default {
        set f [mktemp tf]
        fconfigure $f -translation binary
        lappend temporaryFiles $tf
        puts -nonewline $f [::http::data $tok]
        close $f

        lappend filesToMove $tf $filename
      }
    }

    ::http::cleanup $tok
    downloadNext
  }

  method mergeConfigs {} {
    $ open abendstern.rc.new confnew

    doMergeConfigs conf confnew

    $ close confnew
    $ sync conf
  }

  method doMergeConfigs {dst src} {
    for {set i 0} {$i < [$ length $src]} {incr i} {
      set name [$ name $src.\[$i\]]
      if {![$ exists $dst.$name]} {
        $ add $dst $name [$ getType $src.$name]
        $ copy $dst.$name $src.$name
      } elseif {"STGroup" == [$ getType $src.$name]} {
        doMergeConfigs $dst.$name $src.$name
      }
    }
  }

  method applyNext {} {
    if {![llength $filesToMove]} {
      set reallyNeedRunApplyUpdate $needRunApplyUpdate
      set reallyNeedMergeConfigs $needMergeConfigs
      # Done!
      # Save local manifest
      set man [open manifest w]
      dict for {filename md5} $localManifest {
        puts $man "$md5 $filename"
      }
      close $man

      # Merge any config changes
      if {$reallyNeedMergeConfigs} mergeConfigs

      set isApplying no
      if {$needMergeConfigs} {
        set confmsg "New configuration information has been merged into your preferences file."
      } else {
        set confmsg {}
      }

      if {$needRunApplyUpdate} {
        set aumsg [string map [list "`" "\n"] [concat \
          "`\a\[(white)IMPORTANT:\a\] When you exit, a Command Prompt window will appear." \
          "Please wait until five seconds after Abendstern has exited,"\
          "then follow its directions. This is required to finish updating files which"\
          "Abendstern cannot modify while it is running."]]
      } else {
        set aumsg {}
      }
      displayMessage "Updates Applied" [string map [list "`" "\n"] [concat \
        "Updates have been applied. You must restart Abendstern for" \
        "the changes to take effect." \
        $confmsg \
        $aumsg \
        "`Press Close to exit Abendstern now."]]
      return
    }

    set filesToMove [lassign $filesToMove src dst]
    set dir [file dirname $dst]
    if {$dir != "."} {
      file mkdir $dir
    }

    if {[file exists $dst]} {
      file delete $dst
    }

    file copy -force $src $dst

    incr currProgress
    $ptotal setProgress [expr {$currProgress*2.0/$maxProgress}]
    $lfile setText $dst
  }

  method displayMessage {t m} {
    after 100 "$this displayMessage_impl {$t} {$m}"
  }
  method displayMessage_impl {title message} {
    $mainpanel setElt centre [new ::gui::MultiLineLabel $message]
    $ltitle setText $title
    $bclose setText Close
    set isApplying no
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method abort {} {
    if {$isApplying} {
      # Not safe to exit
      return
    }

    # Delete temporary files
    foreach file $temporaryFiles {
      file delete $file
    }

    if {$reallyNeedRunApplyUpdate} {
      # We can't just do
      #  exec apply_update.bat &
      # because (a) it gets bound to the non-existent
      # standard input, and
      # (b) PAUSE terminates immediately, so the copy
      # fails.
      # START, as of the Windows NT line, seems to
      # be a command of CMD.EXE instead of being
      # its own executable (?!)
      exec cmd.exe /C START apply_update.bat &
    }

    # We have to do a fast shutdown without using
    # BootManager since the shutdown scripts may
    # have been altered (and therefore might not
    # be compatible with what is currently running.)
    $::state configure -retval $::state
  }
}

# See if any updates are available
proc receiveCurrentUpdateVersion tok {
  if {"ok" == [::httpa::status $tok] && 200 == [::httpa::ncode $tok]} {
    catch {
      set f [open version r]
      set localVersion [string trim [read $f]]
      close $f
      if {$localVersion != [string trim [::httpa::data $tok]]} {
        ::gui::setNotification "\a\[(white)Updates are available!\a\]"
      }
    }
  }
  ::httpa::cleanup $tok
}

::httpa::geturl \
  "http://$::abnet::SERVER:12544/abendstern/package/version" \
  -timeout 2500 \
  -command receiveCurrentUpdateVersion
