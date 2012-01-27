# Defines the Login Application, which allows the user to log in,
# create an account, or perform a local login.
# When login is done, the Login state terminates (it is intended
# to be run by BootManager)
#
# It has the following Modes:
# + LoginPrompt -- Prompts the user to log in, create an account,
#                  or perform a local login (requiring confirmation)
# + LoginOfflineAdvisory -- Informs the user of the disadvantages
#                           of offline logins. If offline login was
#                           automatic, the user is also informed of
#                           this and given the option to not show
#                           the dialogue in the future (only for
#                           automatics).

# Automatic login is disabled if a login has been run before
# (this both makes the "log out" function work, and prevents
#  an infinite loop if the stored credentials are invalid)
set permitAutomaticLogin yes

class Login {
  inherit ::gui::Application

  variable alive

  constructor {} {
    ::gui::Application::constructor
  } {
    set alive yes

    if {$::abnet::isConnected} {
      set mode [new LoginPrompt $this]
    } elseif {[$ exists conf.suppress_auto_offline_note]
          &&  [$ bool   conf.suppress_auto_offline_note]} {
      ::abnet::offlineLogin
      set alive no
      # Just because we do need SOME mode
      set mode [new LoginOfflineAdvisory $this yes]
    } else {
      set mode [new LoginOfflineAdvisory $this yes]
    }
  }

  method updateThis et {
    $mode update $et
    ::sbg::update $et
    if {$alive} {
      return 0
    } else {
      return $this
    }
  }

  method drawThis {} {
    ::sbg::draw
  }

  method finished {} {
    set alive no
  }

  method setMode m {
    lappend ::gui::autodelete $mode
    set mode $m
  }

  method getCursor {} {
    if {$::abnet::busy} {
      return busy
    } else {
      return crosshair
    }
  }
}

# The primary login window.
# Layout:
#
#             "Abendstern Network Login"
# Username: [                                   ]
# Password: [***********************************]
# Password: [***********************************]
# [x] Log into this account automatically
# <[(Log in)]> <[Create new account]> <[Play offline]>
#
# The second password field is only shown in create new account
# mode; the title is used to show status.
class LoginPrompt {
  inherit ::gui::Mode

  variable app

  variable ltitle     ;# Displays current operation, or result of previous
  variable fusername  ;# Name of user to create/log in to
  variable fpassword0 ;# Primary password entry
  variable fpassword1 ;# Confirm password for creation mode
  variable lpassword1 ;# Blank label to show in login mode
  variable ppassword1 ;# Simple panel to allow swapping the contents of password1
  variable cautomatic ;# If checked (not by default), store automatic login information
  variable bactivate  ;# Log in or create account
  variable btoggle    ;# Switch between log in and creation mode

  variable mode ;# "login" or "create"
  variable busy ;# If true, we are waiting for an operation to complete and have controls disabled
  variable wasPasswordTranslationEnabled ;# Whether the most recent op used pw translation

  constructor par {
    ::gui::Mode::constructor
  } {
    set app $par
    set ltitle [new ::gui::Label [_ A login tlogin] centre]
    set fusername [new ::gui::TextField [_ A login username] {} {} {} \
                   [list $this usernameAction] [list $this usernameChanged]]
    set fpassword0 [new ::gui::TextField [_ A login password] {} {} {} \
                    [list $this password0Action] [list $this passwordChanged]]
    set fpassword1 [new ::gui::TextField [_ A login password] {} {} {} \
                    [list $this password1Action] [list $this passwordChanged]]
    $fpassword0 setObscured yes
    $fpassword1 setObscured yes
    set lpassword1 [new ::gui::Label {}]
    set ppassword1 [new ::gui::TabPanelViewer]
    $ppassword1 add $fpassword1
    $ppassword1 add $lpassword1
    set cautomatic [new ::gui::Checkbox [_ A login automatic]]
    $cautomatic setChecked [expr {[$ exists conf.auto_login.pass] && $::permitAutomaticLogin}]
    set bactivate [new ::gui::Button [_ A login login] [list $this primaryAction]]
    $bactivate setDefault
    set btoggle [new ::gui::Button [_ A login switchcreate] [list $this toggleMode]]
    set boffline [new ::gui::Button [_ A login offline] [list $this loginOffline]]

    set buttons [new ::gui::HorizontalContainer 0.01 centre]
    $buttons add $bactivate
    $buttons add $btoggle
    $buttons add $boffline
    set main [new ::gui::VerticalContainer 0.01]
    foreach component [list \
        $ltitle $fusername $fpassword0 $ppassword1 \
        $cautomatic $buttons] {
      $main add $component
    }

    set root [new ::gui::ComfyContainer [new ::gui::Frame $main] 0.6]

    refreshAccelerators
    $root setSize 0 1 $::vheight 0
    $root packy

    # This must be done AFTER size is set
    $ppassword1 setActive $lpassword1

    set mode login
    set busy no

    if {$::permitAutomaticLogin && [$ exists conf.auto_login.pass]} {
      $fusername  setText [$ str conf.auto_login.name]
      $fpassword0 setText [$ str conf.auto_login.pass]
      primaryAction no ;# The stored password contains the hashed value
    } elseif {[$ exists conf.auto_login.name]} {
      $fusername setText [$ str conf.auto_login.name]
      $fusername gainFocus
    } else {
      $fusername gainFocus
    }

    set ::permitAutomaticLogin no
  }

  method update et {
    if {!$::abnet::isConnected} {
      # This is a problem, retry connection
      log $::abnet::resultMessage
      $::state setCatalogue login
      $app finished
      return
    }
    if {$busy && !$::abnet::busy} {
      # Operation completed, indicate status to user
      set busy no
      $fusername setEnabled yes
      $fpassword0 setEnabled yes
      $fpassword1 setEnabled yes

      if {$::abnet::success} {
        # Save last username if it does not
        # interfere with an automatic login
        # If automatic is set, set both
        if {![$ exists conf.auto_login]} {
          $ add conf auto_login STGroup
          $ add conf.auto_login name STString
        }
        if {![$ exists conf.auto_login.pass] || [$cautomatic isChecked]} {
          # Save name
          $ sets conf.auto_login.name [$fusername getText]
        }
        if {[$cautomatic isChecked]} {
          # Save password
          if {![$ exists conf.auto_login.pass]} {
            $ add conf.auto_login pass STString
          }
          $ sets conf.auto_login.pass \
            [translatePassword $wasPasswordTranslationEnabled [$fpassword0 getText]]
        }
        $ sync conf

        $app finished
      } else {
        # If this was an automatic login (exact match between stored
        # items and fields), delete the password entry
        if {[$ exists conf.auto_login.pass]
        &&  [$fusername  getText] == [$ str conf.auto_login.name]
        &&  [$fpassword0 getText] == [$ str conf.auto_login.pass]
        &&  $mode == "login"} {
          $ remove conf.auto_login.pass
        }
        # Indicate failure message
        $ltitle setText "\a\[(danger)$::abnet::resultMessage\a\]"
      }
    }
  }

  method primaryAction {{enablePasswordTranslation yes}} {
    if {$busy} return

    if {$mode == "login"} {
      $ltitle setText [_ A login logging_in]

      ::abnet::login [$fusername getText] \
        [translatePassword $enablePasswordTranslation [$fpassword0 getText]]
    } else {
      if {![passwordsValid]} return

      ::abnet::createAcct [$fusername getText] \
        [translatePassword $enablePasswordTranslation [$fpassword0 getText]]

      $ltitle setText [_ A login creating]
    }

    set busy yes
    $fusername setEnabled no
    $fpassword0 setEnabled no
    $fpassword1 setEnabled no
    set wasPasswordTranslationEnabled $enablePasswordTranslation
  }

  method toggleMode {} {
    if {$busy} return

    if {$mode == "login"} {
      set mode create
      $ppassword1 setActive $fpassword1
      $bactivate setText [_ A login create]
      $btoggle setText [_ A login switchlogin]
      $ltitle setText [_ A login tcreate]
      refreshAccelerators
      $root packy
    } else {
      set mode login
      $ppassword1 setActive $lpassword1
      $bactivate setText [_ A login login]
      $btoggle setText [_ A login switchcreate]
      $ltitle setText [_ A login tlogin]
      refreshAccelerators
      $root packy
    }
  }

  method loginOffline {} {
    $app setMode [new LoginOfflineAdvisory $app no]
  }

  method usernameChanged args {
    return yes
  } ;# Currently does nothing

  method passwordChanged args {
    after idle [list $this passwordsValid]
    return yes
  }

  # Checks to see if the passwords are valid (in creation mode)
  # and indicates the status in the title
  # Returns whether they are acceptable (long enough and match)
  method passwordsValid {} {
    if {$mode != "create"} { return yes }

    if {[$fpassword0 getText] != [$fpassword1 getText]} {
      $ltitle setText "[_ A login tcreate] \a\[(warning)[_ A login pwnotmatch]\a\]"
      return no
    }
    set pw [$fpassword0 getText]
    if {[string length $pw] < 7} {
      $ltitle setText "[_ A login tcreate] \a\[(warning)[_ A login pwshort]\a\]"
      return no
    }

    $ltitle setText [_ A login tcreate]
    return yes
  }

  # Returns the input if enabled is false;
  # if enabled is true, returns a SHA-256 hash
  # of the input
  method translatePassword {enabled pw} {
    if {$enabled} {
      return [::sha2::sha256 $pw]
    } else {
      return $pw
    }
  }

  method usernameAction args {
    $fpassword0 gainFocus
  }

  method password0Action args {
    if {$mode == "login"} {
      primaryAction yes
    } else {
      $fpassword1 gainFocus
    }
  }

  method password1Action args {
    primaryAction yes
  }
}

# The LoginOfflineAdvisory tells the user about the disadvantages
# of not playing offline.
# If automatic is no, they are given the option to proceed or
# cancel; if automatic is yes, they are told that the network is
# not available and given the option to not show the messsage in
# the future.
class LoginOfflineAdvisory {
  inherit ::gui::Mode

  variable cdonotshow
  variable app

  constructor {par automatic} {
    ::gui::Mode::constructor
  } {
    set app $par
    set main [new ::gui::BorderContainer 0 0.01]
    if {$automatic} {
      set txt "[_ A login offline_auto] [_ A login offline_disadvantages]"
    } else {
      set txt [_ A login offline_disadvantages]
    }
    $main setElt top [new ::gui::Label [_ A login toffline] centre]
    $main setElt centre [new ::gui::MultiLineLabel $txt]
    set controls [new ::gui::VerticalContainer 0.01]
    if {$automatic} {
      set cdonotshow [new ::gui::Checkbox [_ A login dont_show_again]]
      $controls add $cdonotshow
    } else {
      set cdonotshow {}
    }

    set buttons [new ::gui::HorizontalContainer 0.01 centre]
    if {$automatic} {
      set ok [new ::gui::Button [_ A gui ok] [list $this ok]]
      $ok setDefault
      $buttons add $ok
    } else {
      set ok [new ::gui::Button [_ A login offline] [list $this ok]]
      set can [new ::gui::Button [_ A gui cancel] [list $this cancel]]
      $can setCancel
      $buttons add $ok
      $buttons add $can
    }

    $controls add $buttons
    $main setElt bottom $controls

    set root [new ::gui::ComfyContainer [new ::gui::Frame $main]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }

  method ok {} {
    if {[string length $cdonotshow] && [$cdonotshow isChecked]} {
      if {![$ exists conf.suppress_auto_offline_note]} {
        $ add conf suppress_auto_offline_note STBool
      }
      $ setb conf.suppress_auto_offline_note yes
    }

    ::abnet::offlineLogin
    $app finished
  }

  method cancel {} {
    $app setMode [new LoginPrompt $app]
  }
}
