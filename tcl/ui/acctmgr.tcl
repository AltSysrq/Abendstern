# Provides the user with a GUI to change their account's
# name or password, or to delete the account.
#
# This must be run as a top-level state, since it may
# invoke BootManager to rerun the login state if the
# connection is lost or account deleted; it also requires
# the tcl/init.d/mainmenu.tcl script to be able to call
# its setReturn method.

# A simple shell around the related modes.
# It has the setReturn method which matches that of
# BootManager's, so that it can use tcl/init.d/mainmenu.tcl
# to reload the main menu when it is closed.
class AccountManager {
  inherit ::gui::Application

  variable retval

  # Constructs a new AccountManager with the given code
  # to create a new Application to switch to when closed
  # (without
  constructor {} {
    ::gui::Application::constructor
  } {
    set retval 0
    set mode [new AccountManagerMode $this]
  }

  method drawThis {} {
    ::sbg::draw
  }

  method updateThis et {
    if {$::abnet::isConnected} {
      ::sbg::update $et
      $mode update $et
    } else {
      set retval [new BootManager login]
    }
    return $retval
  }

  method setReturn r {
    set retval $r
  }

  method getCursor {} {
    if {$::abnet::busy} {
      return busy
    } else {
      return crosshair
    }
  }

  method setMode m {
    lappend ::gui::autodelete $mode
    set mode $m
  }
}

class AccountManagerMode {
  inherit ::gui::Mode

  variable app
  variable ltitle       ;# Title of the window, and current status
  variable fusername    ;# Username, initialised to ::abnet::username
  variable fpassword0   ;# Password (if both are blank, don't change)
  variable fpassword1
  variable lpasswordStat;# Indicate status of passwords
  variable icurrAvatar  ;# The user's current avatar
  variable inewAvatar   ;# Preview of avatar user wants to switch to
  variable lavatars     ;# Possible avatars for the user to switch to

  variable tempAvatarName ;# The file to save the avatar to

  variable oldName

  variable alterationPending

  constructor par {
    ::gui::Mode::constructor
  } {
    set app $par
    set alterationPending no
    set oldName $::abnet::username
    set tempAvatarName [tmpname]

    set top [new ::gui::BorderContainer 0 0.01]
    set ltitle [new ::gui::Label [_ A login tmanagement] centre]
    $top setElt top $ltitle
    set combined [new ::gui::BorderContainer 0 0.01]
    set main [new ::gui::VerticalContainer 0.01]
    set fusername [new ::gui::TextField [_ A login username] $::abnet::username]
    $main add $fusername
    set fpassword0 [new ::gui::TextField [_ A login password] {} {} {} \
                    [list $this focusPassword1] [list $this passwordsChanged]]
    $fpassword0 setObscured yes
    $main add $fpassword0
    set fpassword1 [new ::gui::TextField [_ A login password] {} {} {} \
                    [list $this alter] [list $this passwordsChanged]]
    $fpassword1 setObscured yes
    $main add $fpassword1
    set lpasswordStat [new ::gui::Label [_ A login pwnotchanged] left]
    $main add $lpasswordStat
    set cautomatic [new ::gui::Checkbox [_ A login automatic] \
      {expr {[$ exists conf.auto_login.name]
         &&  $::abnet::username == [$ str conf.auto_login.name]
         &&  [$ exists conf.auto_login.pass]}} \
      {} \
      [list $this enableAuto] [list $this disableAuto]]
    $main add $cautomatic
    $main add [new ::gui::MultiLineLabel [_ A login avatar_howto] 2]
    $combined setElt top $main

    set icurrAvatar [new ::gui::SquareIcon]
    set inewAvatar [new ::gui::SquareIcon]
    set lavatars [new ::gui::List [_ A login possible_avatars] \
      [lsort [glob -nocomplain *.png *.jpg *.jpeg *.bmp *.gif]] \
      no "$this previewNewAvatar"]
    set pright [new ::gui::BorderContainer]
    # Use panels to align the icons to the top and give them some extra padding
    set riap [new ::gui::VerticalContainer]
    $riap add [new ::gui::ComfyContainer $icurrAvatar 0.90]
    set liap [new ::gui::VerticalContainer]
    $liap add [new ::gui::ComfyContainer $inewAvatar 0.90]
    $pright setElt centre $liap
    set bnewAvatar [new ::gui::Button [_ A login change_avatar] "$this setNewAvatar"]
    $pright setElt bottom $bnewAvatar
    $inewAvatar dupeMinWidth $bnewAvatar
    $icurrAvatar dupeMinWidth $bnewAvatar
    set pavatar [new ::gui::BorderContainer 0 0.01]
    $pavatar setElt left $riap
    $pavatar setElt centre $lavatars
    $pavatar setElt right $pright
    $combined setElt centre $pavatar

    $top setElt centre $combined

    set buttons [new ::gui::HorizontalContainer 0 grid]
    set mainButtons [new ::gui::HorizontalContainer 0.01 left]
    $mainButtons add [new ::gui::Button [_ A login alter] [list $this alter]]
    set cancel [new ::gui::Button [_ A gui close] "$app setReturn $app"]
    $cancel setCancel
    $mainButtons add $cancel
    $buttons add $mainButtons
    set delButton [new ::gui::HorizontalContainer 0 right]
    $delButton add [new ::gui::Button [_ A login delete] "$app setMode \[new AMDeleteConfirm $app\]"]
    $buttons add $delButton
    $top setElt bottom $buttons

    set root [new ::gui::ComfyContainer [new ::gui::Frame $top]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0

    # Get the user's current avatar
    ::abnet::getfn avatar $tempAvatarName
  }

  destructor {
    file delete $tempAvatarName
  }

  method update et {
    if {$alterationPending && !$::abnet::busy} {
      # Done, reenable editing of the text fields
      $fusername setEnabled yes
      $fpassword0 setEnabled yes
      $fpassword1 setEnabled yes
      if {$::abnet::success} {
        # Set the title back
        $ltitle setText [_ A login tmanagement]

        # If the old name matches what is stored in auto_login,
        # update that (and the password, if present)
        if {[$ exists conf.auto_login.name]
        &&  $oldName == [$ str conf.auto_login.name]} {
          $ sets conf.auto_login.name $::abnet::username
          if {[$ exists conf.auto_login.pass]} {
            $ sets conf.auto_login.pass $::abnet::password
          }
        }

        set oldName $::abnet::username
      } else {
        # Indicate error, revert username
        $ltitle setText "\a\[(danger)$::abnet::resultMessage\a\]"
        $fusername setText $::abnet::username
      }

      set alterationPending no
    }

    # Update avatar display if needed
    if {!$::abnet::busy && ![$icurrAvatar isLoaded]} {
      if {![$icurrAvatar load $tempAvatarName 64 SILRLax]} {
        $icurrAvatar load images/avatar.png 64 SILRStrict
      }
    }
  }

  method focusPassword1 args {
    $fpassword1 gainFocus
  }

  method passwordsChanged args {
    after idle [list $this validatePasswords]
    return yes
  }

  method validatePasswords {} {
    if {[$fpassword0 getText] != [$fpassword1 getText]} {
      $lpasswordStat setText "\a\[(danger)[_ A login pwnotmatch]\a\]"
      return no
    }
    set pw [$fpassword0 getText]
    if {"" == $pw} {
      $lpasswordStat setText [_ A login pwnotchanged]
      return yes
    }
    if {[string length $pw] < 7} {
      $lpasswordStat setText "\a\[(danger)[_ A login pwshort]\a\]"
      return no
    }

    $lpasswordStat setText [_ A login pwok]
    return yes
  }

  method alter args {
    if {![validatePasswords]} return
    set alterationPending yes
    $fusername setEnabled no
    $fpassword0 setEnabled no
    $fpassword1 setEnabled no

    if {[string length [$fpassword0 getText]]} {
      # Changing the password, hash it
      set pw [::sha2::sha256 [$fpassword0 getText]]
    } else {
      # Using current password
      set pw $::abnet::password
    }

    $ltitle setText [_ A login altering]
    ::abnet::alterAcct [$fusername getText] $pw
  }

  method disableAuto {} {
    $ remove conf.auto_login.pass
  }
  method enableAuto {} {
    if {![$ exists conf.auto_login]} { $ add conf auto_login STGroup }
    if {![$ exists conf.auto_login.name]} {
      $ add conf.auto_login name STString
    }
    if {![$ exists conf.auto_login.pass]} {
      $ add conf.auto_login pass STString
    }
    $ sets conf.auto_login.name $::abnet::username
    $ sets conf.auto_login.pass $::abnet::password
  }

  method previewNewAvatar {} {
    set ix [$lavatars getSelection]
    if {![llength $ix]} return
    $inewAvatar load [lindex [$lavatars getItems] $ix] 64 SILRScale
  }

  method setNewAvatar {} {
    if {![$inewAvatar isLoaded]} return
    # Save file and upload
    if {[$inewAvatar save $tempAvatarName]} {
      $icurrAvatar unload
      ::abnet::putf avatar $tempAvatarName yes
    }
  }
}

# Ensures that the user is sure that they want to delete their account
class AMDeleteConfirm {
  inherit ::gui::Mode

  constructor app {
    ::gui::Mode::constructor
  } {
    set main [new ::gui::BorderContainer 0 0.01]
    $main setElt top [new ::gui::Label [_ A login tdelete]]
    $main setElt centre [new ::gui::MultiLineLabel [_ A login deletewarn]]
    set buttons [new ::gui::HorizontalContainer 0.01 centre]
    set ok [new ::gui::Button [_ A gui yes] ::abnet::deleteAcct]
    $buttons add $ok
    set can [new ::gui::Button [_ A gui no] "$app setMode \[new AccountManagerMode $app\]"]
    $can setCancel
    $buttons add $can
    $main setElt bottom $buttons

    set root [new ::gui::ComfyContainer [new ::gui::Frame $main]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0
  }
}
