# Allows the user to select a game mode and start a new game, or join an
# existing one.
class GameGUI {
  inherit gui::Application

  variable ret

  constructor {} {
    Application::constructor
  } {
    set mode [new GameGUIMode $this]
    set ret 0
  }

  method updateThis {et} {
    ::sbg::update $et
    $mode update $et
    return $ret
  }

  method drawThis {} {
    ::sbg::draw
  }

  method die {} { set ret $this }
  method setRet {what} {
    set ret $what
  }
}

class GameGUIMode {
  inherit ::gui::Mode

  variable fscreenName
  variable fipAddress
  variable fport
  variable lstlanGames
  variable lhangars

  variable app
  variable network
  variable dummyField

  public variable stdhangar yes

  constructor {app_} {
    Mode::constructor
  } {
    set dummyField [new GameField default 1 1]
    set app $app_
    set network [new NetworkGame default $dummyField]

    set main [new gui::TabPanel yes]
    set plocalTop [new gui::BorderContainer 0 0.01]
    set plocal [new gui::VerticalContainer 0.01]
    $plocal add [new gui::Label [_ A gamegui background] left]
    set prevbox none
    foreach bg {StarField Planet Nebula} {
      set box [new ::gui::RadioButton [_ A gamegui $bg] \
               "expr {\[$ str conf.game.background\] == {$bg}}" \
               "$ sets conf.game.background $bg" \
               $prevbox]
      $plocal add $box
      set prevbox $box
    }

    $plocal add [new gui::Label [_ A gamegui difficulty] left]
    set prevbox none
    foreach {ix val} {0 0.25 1 0.5 2 1.0 3 2.5 4 10.0} {
      set box [new ::gui::RadioButton [_ A gamegui difficulty_$ix] \
               "expr {\[$ float conf.game.dmgmul\] == {$val}}" \
               "$ setf conf.game.dmgmul $val" \
               $prevbox]
      $plocal add $box
      set prevbox $box
    }

    $plocalTop setElt centre $plocal

    set startlocal [new gui::Button [_ A gamegui startlocal] "$this startLocal"]
    $startlocal setDefault
    $plocalTop setElt bottom $startlocal
    $main add [_ A gamegui tab_local] $plocalTop

    set planTop [new ::gui::BorderContainer 0 0.01]
    set planUpper [new ::gui::VerticalContainer 0.01]
    set fscreenName [new ::gui::TextField [_ A gamegui screenname] \
                         $::abnet::username]
    $planUpper add $fscreenName
    $planTop setElt top $planUpper

    set pjoinlst [new ::gui::BorderContainer]
    set lstlanGames [new ::gui::List [_ A gamegui langamelist]]
    $pjoinlst setElt centre $lstlanGames
    set pjoinlstbuttons [new ::gui::HorizontalContainer 0.01 right]
    $pjoinlstbuttons add [new ::gui::Button [_ A gamegui refresh] \
                              "$this refreshLan"]
    $pjoinlstbuttons add [new ::gui::Button \
                              [_ A gamegui join_from_list] \
                              "$this joinLanGameFromList"]
    $pjoinlst setElt bottom $pjoinlstbuttons
    $planTop setElt centre [new ::gui::Frame $pjoinlst]

    set planBot [new ::gui::VerticalContainer 0.01]
    set planspec [new ::gui::VerticalContainer 0.01]
    set fipAddress [new ::gui::TextField [_ A gamegui ip_address] {}]
    $planspec add $fipAddress
    set fport [new ::gui::TextField [_ A gamegui port] 12544]
    $planspec add $fport
    $planspec add [new ::gui::Button [_ A gamegui join_specified] \
                       "$this joinLanSpecified"]
    $planBot add [new ::gui::Frame $planspec]

    $planBot add [new ::gui::Button [_ A gamegui startlan] \
                      "$this startLanGame"]
    $planTop setElt bottom $planBot
    $main add [_ A gamegui tab_lan] $planTop

    # Game parameters
    set gameparms [new ::gui::VerticalContainer 0.01]
    set boxlists [new ::gui::HorizontalContainer 0.01 grid]
    set modeboxen [new ::gui::VerticalContainer 0.01]
    set prevbox none
    foreach mode {dm xtdm lms lxts hvc wlrd} {
      set box [new ::gui::RadioButton [format [_ A game "g_${mode}_long"] X] \
               "expr {\[$ str conf.game.mode\] == {$mode}}" \
               "$ sets conf.game.mode $mode" \
               $prevbox]
      $modeboxen add $box
      set prevbox $box
    }
    $boxlists add $modeboxen

    set classboxen [new ::gui::VerticalContainer 0.01]
    set prevbox none
    foreach class {C B A} {
      set box [new ::gui::RadioButton \
                   [format "%s%s" [_ A editor class_prefix] $class] \
                   "expr {\[$ str conf.game.class\] == {$class}}" \
                   "$ sets conf.game.class $class" \
                   $prevbox]
      $classboxen add $box
      set prevbox $box
    }
    $boxlists add $classboxen
    $gameparms add $boxlists

    $gameparms add [new ::gui::Slider [_ A gamegui size] int \
                    "$ int conf.game.size" \
                    "$ seti conf.game.size" \
                    16 256 \
                    8 {} \
                    "format %d" [$::gui::font width 9999]]
    $gameparms add [new ::gui::Slider [_ A gamegui numais] int \
                    "$ int conf.game.nai" \
                    "$ seti conf.game.nai" \
                    1 64 \
                    1 {} \
                    "format %d" [$::gui::font width 199]]
    $gameparms add [new ::gui::Slider [_ A gamegui nteams] int \
                    "$ int conf.game.nteams" \
                    "$ seti conf.game.nteams" \
                    2 6 \
                    1 {} \
                    "format %d" [$::gui::font width 6]]
    $main add [_ A gamegui tab_parm] $gameparms

    set aihangartop [new ::gui::BorderContainer 0 0.01]
    set aihangar [new ::gui::VerticalContainer 0.01]
    set aibestships [new ::gui::RadioButton [_ A gamegui ai_bestships] \
                         {expr 1} "$this configure -stdhangar yes" none]
    set aicustom [new ::gui::RadioButton [_ A gamegui ai_customships] \
                      {expr 0} "$this configure -stdhangar no" $aibestships]
    $aihangar add $aibestships
    $aihangar add $aicustom
    $aihangartop setElt top $aihangar
    set lhangars [new ::gui::List [_ A gamegui ai_customhangar] \
                      [getHangarList] no "$this setAiHangar"]
    $aihangartop setElt centre $lhangars

    $main add [_ A gamegui tab_aisettings] $aihangartop

    set top [new ::gui::BorderContainer 0]
    $top makeCentreLast
    $top setElt centre $main
    set cancel [new ::gui::Button [_ A gui cancel] "$app die"]
    $cancel setCancel
    set pcancel [new ::gui::HorizontalContainer 0 right]
    $pcancel add $cancel
    $top setElt bottom $pcancel

    set root [new ::gui::ComfyContainer [new ::gui::Frame $top]]
    refreshAccelerators
    $root setSize 0 1 $::vheight 0

    refreshLan
  }

  destructor {
    delete object $dummyField
  }

  method getHangarList {} {
    set lst {}
    conffor entry hangar.user {
      lappend lst [$ str $entry.name]
    }
    return $lst
  }

  method setAiHangar {} {
    set sel [$lhangars getSelection]
    if {$sel ne {}} {
      makeHangarEffective $sel
    }
  }

  method update et {
    if {$network != 0} {
      $network update [expr {int($et)}]
      set sel [$lstlanGames getSelection]
      set items [$network getDiscoveryResults]
      $lstlanGames setItems $items
      if {[llength $sel] > 0 && $sel >= 0 && $sel < [llength $items]} {
        $lstlanGames setSelection $sel
      }
    }
  }

  method startLocal {} {
    delete object $network
    set network 0
    changeScreenName
    lassign [getGameStateArgs 0] modestr background
    $app setRet [new GameManager 0 \
                     [list init-local-game $modestr] $background $stdhangar]
  }

  method joinLanGameFromList {} {
    if {![llength [$lstlanGames getSelection]]} return
    changeScreenName
    lassign [getGameStateArgs $network] modestr background
    $app setRet [new GameManager $network \
                     [list join-lan-game 1 [$lstlanGames getSelection]] \
                     $background $stdhangar]
  }

  method joinLanSpecified {} {
    changeScreenName
    set addr [$fipAddress getText]
    set addr [string trim $addr]
    if {$addr eq ""} return
    set port [string trim [$fport getText]]
    if {![string is integer -strict $port]} return
    lassign [getGameStateArgs $network] modestr background
    $app setRet [new GameManager $network \
                     [list join-private-game 0 $addr $port] \
                     $background $stdhangar]
  }

  method startLanGame {} {
    changeScreenName

    lassign [getGameStateArgs $network] modestr background
    $app setRet [new GameManager $network \
                     [list init-lan-game 4 1 $modestr] \
                     $background $stdhangar]
  }

  # Returns a two-item list of the modestr,background to use
  private method getGameStateArgs {network} {
    $root save
    if {$network eq 0} {
      set bkgr [$ str conf.game.background]
    } else {
      set bkgr StarField
    }

    switch -exact -- $bkgr {
      StarField {
        set background {new StarField default 0 $field 1000}
      }
      Planet {
        set background {new Planet default 0 $field \
                        "images/earthday_post.png" \
                        "images/earthnight_post.png" \
                        -900000 -180000 2.5 0.1}
      }
      Nebula {
        set background {
          set r [expr {rand()}]
          set g [expr {rand()}]
          set b [expr {rand()}]
          set viscosity [expr {rand()*rand()}]
          set density [expr {20*rand()}]
          set nebula [new Nebula default 0 $field $r $g $b $viscosity $density]
          set xxc [expr {20*rand()}]
          set xyc [expr {20*rand()}]
          set yxc [expr {20*rand()}]
          set yyc [expr {20*rand()}]
          set xf [expr {rand()}]
          set yf [expr {rand()}]
          $nebula setFlowEquation "* $xf + cos * x $xxc cos * y $yyc" \
                                  "* $yf + cos * x $yxc cos * y $yyc" yes
          $nebula setVelocityResetTime [expr {200*rand()}]
          $nebula setForceMultiplier [expr {rand()}]
          expr {$nebula}
        }
      }
    }
    set opts [dict create \
                  fieldw [$ int conf.game.size] \
                  fieldh [$ int conf.game.size] \
                  desiredPlayers [expr {[$ int conf.game.nai]+1}]]
    switch -exact -- [$ str conf.game.mode] {
      dm {
        set ms DM__
      }
      xtdm {
        set ms [format %dTDM [$ int conf.game.nteams]]
      }
      lms {
        set ms LMS_
      }
      lxts {
        set ms [format L%dTS [$ int conf.game.nteams]]
      }
      hvc {
        set ms HVC_
      }
      wlrd {
        set ms WLRD
      }
    }

    set cls [$ str conf.game.class]
    set modestr [list "${ms}${cls}0" $opts]
    return [list $modestr $background]
  }

  method refreshLan {} {
    if {[$network discoveryScanProgress] < 0 || [$network discoveryScanDone]} {
      $network startDiscoveryScan
    }
  }

  # Changes the screen name if the current user is local.
  # The new name will always start with ~.
  method changeScreenName {} {
    if {"~" ne [string index $::abnet::username 0]} {
      return ;# Logged in, cannot rename
    }

    set name [string trim [$fscreenName getText]]
    if {"~" ne [string index $name 0]} {
      set name "~$name"
    }
    if {[string length $name] < 4} return

    set ::abnet::username $name
  }
}
