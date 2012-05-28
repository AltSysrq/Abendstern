# Runs any code needed when starting preliminary run mode, and provides the
# PrelimRunApp to allow the user to configure Abendstern before starting the
# game proper.

# On Windows, start all three versions with --help (so they exit immediately)
# so that the Windows firewall will prompt the user before any fullscreen
# window is created.
if {$PLATFORM == "WINDOWS"} {
  foreach v {14 21 32} {
    catch {
      exec "bin\\abw32gl$v.exe" --help
    }
  }
}

# If the config entry "prelim_assume_version" exists, immediately finish prelim
# mode for that version.
# This is different from "skip_prelim" (which is not exposed to the user),
# which simply suppresses running of preliminary mode.
#
# Note that the user interface only creates prelim_assume_version_pre, which
# must be renamed by Abendstern only on successful shutdown.
if {[$ exists conf.prelim_assume_version]} {
  if {[catch {
    set recommendedGLType [$ str conf.prelim_assume_version]
    exitPreliminaryRunMode
  } err]} {
    # Could only happen if we couldn't set the type.
    log "Error in conf.prelim_assume_version: $err"
  }

  # Should never get here!
  # But we could, if that version doesn't exist
  log "Error: exitPreliminaryRunMode failed!"
  # Remove the assumption
  $ remove conf.prelim_assume_version
}

# Remove any _pre if it exists
catch {
  $ remove conf.prelim_assume_version_pre
}

class PrelimRunApp {
  inherit ::gui::Application

  public variable ret

  constructor {} {
    ::gui::Application::constructor
  } {
    set mode [new PrelimRunMode $this]
    set ret 0
  }

  method updateThis et {
    ::sbg::update $et
    return $ret
  }

  method drawThis {} {
    ::sbg::draw
  }
}

class PrelimRunMode {
  inherit ::gui::Mode

  variable app
  variable lstlanguage

  constructor {app_} {
    ::gui::Mode::constructor
  } {
    set app $app_

    set top [new ::gui::TabPanel]
    set main [new ::gui::BorderContainer 0 0.01]
    set mainopts [new ::gui::HorizontalContainer 0.01 grid]
    set versions [new ::gui::VerticalContainer 0.01]
    $versions add [new ::gui::Label [_ P version header] left]
    set prevbox none
    foreach v {AGLT32 AGLT21 AGLT14} {
      set box [new ::gui::RadioButton [_ P version $v] \
                   "expr [isVersionUsed $v]" {} $prevbox \
                   "$this setVersion $v"]
      $versions add $box
      set prevbox $box
    }
    $mainopts add $versions

    set lstlanguage [new ::gui::List [_ P main language] [getLanguages] no \
                         "$this changeLanguage"]
    selectCurrentLanguage
    $mainopts add $lstlanguage

    $main setElt centre $mainopts

    set mainbuttons [new ::gui::VerticalContainer 0.01]
    set start [new ::gui::Button [_ P main start] "$this start"]
    $start setDefault
    $mainbuttons add $start
    $mainbuttons add [new ::gui::Checkbox [_ P version always_assume] {expr 0} \
                          {} "$this setAlwaysAssume" "$this clearAlwaysAssume"]
    if {[$ exists conf.auto_login.pass]} {
      $mainbuttons add [new ::gui::Button [_ P main suppress_autologin] \
                            "$this suppressAutologin"]
    }
    set exit [new ::gui::Button [_ A gui quit] "$app configure -ret $app"]
    $exit setCancel
    $mainbuttons add $exit
    $main setElt bottom $mainbuttons
    $top add [_ P tabs main] $main

    set root $top
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method isVersionUsed {v} {
    expr {$v == $::recommendedGLType}
  }

  method getLanguages {} {
    if {[catch {
      set f [open data/lang/manifest r]
      set langs [read $f]
      close $f
    } err]} {
      log "Could not load language manifest: $err"
      return {}
    }

    set ret {}
    foreach {locale name} $langs {
      lappend ret "$name\a\a$locale"
    }

    lsort -dictionary $ret
  }

  method selectCurrentLanguage {} {
    set items [$lstlanguage getItems]
    set currlang [$ str conf.language]
    for {set i 0} {$i < [llength $items]} {incr i} {
      if {[string match "*\a\a$currlang" [lindex $items $i]]} {
        $lstlanguage setSelection $i
        return
      }
    }
  }

  method changeLanguage {} {
    set sel [$lstlanguage getSelection]
    if {$sel == {}} return

    $ sets conf.language [lindex \
                              [split \
                                   [lindex [$lstlanguage getItems] $sel] \
                                   "\a\a"] 1]
  }

  method start {} {
    $ sync conf
    exitPreliminaryRunMode
  }

  method suppressAutologin {} {
    catch {
      $ remove conf.auto_login.pass
    }
  }

  method setVersion v {
    set ::recommendedGLType $v
    if {[$ exists conf.prelim_assume_version_pre]} {
      $ sets conf.prelim_assume_version_pre $v
    }
  }

  method setAlwaysAssume {} {
    $ adds conf prelim_assume_version_pre $::recommendedGLType
  }

  method clearAlwaysAssume {} {
    $ remove conf.prelim_assume_version_pre
  }
}
