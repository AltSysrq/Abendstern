package require Itcl
set applicationUpdateTime 0
# In case anyone needs the current modifiers, the default
# Application keyboard updates this
set currentKBMods ----

namespace eval gui {
  namespace import ::itcl::*
  set version 0.1

  # The standard font to use
  set font $::sysfont

  set needAcceleratorRefresh yes

  # The current cursor to use.
  # Currently supported are:
  #   crosshair         busy
  set cursorType crosshair

  # If non-negative, the cursor will be prevented from moving
  # away from this coordinate on the X axis.
  set cursorLockX -1
  # If non-negative, the cursor will be prevented from moving
  # away from this coordinate on the Y axis.
  set cursorLockY -1

  # All objects in this list are automatically deleted by the application
  # on the start of new frames
  set autodelete {}

  # Notification message to show at the bottom of the screen
  set notification {}
  set notificationQueue {}
  set timeUntilNotificationClear 0
  # Sets the current notification message
  proc setNotification str {
    if {"" == $::gui::notification} {
      set ::gui::notification $str
      set ::gui::timeUntilNotificationClear 4096
    } else {
      lappend ::gui::notificationQueue $str
    }
  }

  # This package defines the GUI system for Abendstern. The top level
  # is arranged into Applications, where an Application is anything like
  # the main menu, the game itself, the Ship editor, etc. An Application
  # is really a glorified GameState; it defines one or more Modes,
  # as well as a background draw operation. Each Mode defines the
  # following:
  # + More keybindings
  # + Whether the mouse cursor should be displayed
  # + How to handle mouse events
  # An Application has exactly one active Mode. It can also run a sub-
  # application, another Application that has full control until it
  # exits by returning itself from its update method.
  #
  # The rest of the GUI is formed via widgets (class AWidget), some of
  # which are containers.

  # Utility functions to set GL to standard colours
  proc getStdColourDirect {name {mult 1.0} {solid no}} {
    set base conf.hud.colours.$name
    list      [expr {[$ float $base.\[0\]]*$mult}] \
              [expr {[$ float $base.\[1\]]*$mult}] \
              [expr {[$ float $base.\[2\]]*$mult}] \
              [expr {$solid? 1.0 : [$ float $base.\[3\]]}]
  }
  proc getStdColour {name {mult 1.0} {solid no}} {
    if {![info exists ::gui::colourCache($name)]} {
      return [getStdColourDirect $name $mult $solid]
    }
    set cached $::gui::colourCache($name)
    list [expr {[lindex $cached 0]*$mult}] \
         [expr {[lindex $cached 1]*$mult}] \
         [expr {[lindex $cached 2]*$mult}] \
         [expr {$solid? 1.0 : [lindex $cached 3]}]
  }
  proc getColourStd  {{m 1.0} {s no}} { getStdColour standard $m $s }
  proc getColourWarn {{m 1.0} {s no}} { getStdColour warning  $m $s }
  proc getColourDang {{m 1.0} {s no}} { getStdColour danger   $m $s }
  proc getColourSpec {{m 1.0} {s no}} { getStdColour special  $m $s }
  proc getColourWhit {{m 1.0} {s no}} { getStdColour white    $m $s }
  proc colourStd   {{m 1.0} {s no}} { glColour {*}[getColourStd  $m $s]}
  proc colourWarn  {{m 1.0} {s no}} { glColour {*}[getColourWarn $m $s]}
  proc colourDang  {{m 1.0} {s no}} { glColour {*}[getColourDang $m $s]}
  proc colourSpec  {{m 1.0} {s no}} { glColour {*}[getColourSpec $m $s]}
  proc colourWhit  {{m 1.0} {s no}} { glColour {*}[getColourWhit $m $s]}
  proc ccolourStd  {{m 1.0} {s no}} {cglColour {*}[getColourStd  $m $s]}
  proc ccolourWarn {{m 1.0} {s no}} {cglColour {*}[getColourWarn $m $s]}
  proc ccolourDang {{m 1.0} {s no}} {cglColour {*}[getColourDang $m $s]}
  proc ccolourSpec {{m 1.0} {s no}} {cglColour {*}[getColourSpec $m $s]}
  proc ccolourWhit {{m 1.0} {s no}} {cglColour {*}[getColourWhit $m $s]}

  # Simple blending
  # Usage: getBlendedColours name mult name mult ...
  # ie, getBlendedColours standard 0.3 special 0.7
  proc getBlendedColours args {
    set blended {0.0 0.0 0.0 0.0}
    foreach {name mult} $args {
      set c [getStdColour $name $mult]
      for {set i 0} {$i < 4} {incr i} {
        set blended [lreplace $blended $i $i [expr {[lindex $blended $i]+[lindex $c $i]}]]
      }
    }
    return $blended
  }
  proc blendedColours args {
    eval "glColor4f [eval "getBlendedColours $args"]"
  }

  # Draw the cursor
  proc drawCursor {} {
    global cursorX cursorY screenW screenH vheight
    set drawX [expr {$cursorX/double($screenW)}]
    set drawY [expr {$vheight - $vheight*$cursorY/double($screenH)}]
    switch -exact -- $::gui::cursorType {
      crosshair {
        set dxl [expr {$drawX-0.01}]
        set dxr [expr {$drawX+0.01}]
        set dyb [expr {$drawY-0.01}]
        set dyt [expr {$drawY+0.01}]
        colourWhit
        glBegin GL_LINES
          glVertex2f $dxl $drawY
          glVertex2f $dxr $drawY
          glVertex2f $drawX $dyb
          glVertex2f $drawX $dyt
        glEnd
      }
      busy {
        # Hourglass (old-fassioned style)
        set dyt [expr {$drawY+0.01}]
        set dyb [expr {$drawY-0.01}]
        set dyht [expr {$drawY+0.005}]
        set dyhb [expr {$drawY-0.005}]
        set dxfl [expr {$drawX-0.0075}]
        set dxml [expr {$drawX-0.005}]
        set dxmr [expr {$drawX+0.005}]
        set dxfr [expr {$drawX+0.0075}]
        set dxhl [expr {$drawX-0.0025}]
        set dxhr [expr {$drawX+0.0025}]
        colourDang 1.0 yes ;# No transparency in cursor
        glBegin GL_TRIANGLES
          glVertex2f $drawX $drawY
          glVertex2f $dxhl $dyht
          glVertex2f $dxhr $dyht

          glVertex2f $dxml $dyb
          glVertex2f $dxmr $dyb
          glVertex2f $drawX $dyhb
        glEnd
        glBegin GL_LINES
          glVertex2f $drawX $drawY
          glVertex2f $drawX $dyhb
          colourWhit
          glVertex2f $dxfl $dyt
          glVertex2f $dxfr $dyt
          glVertex2f $dxfl $dyb
          glVertex2f $dxfr $dyb
          glVertex2f $dxml $dyt
          glVertex2f $dxmr $dyb
          glVertex2f $dxmr $dyt
          glVertex2f $dxml $dyb
          glVertex2f $dxml $dyt
          glVertex2f $dxml $dyb
          glVertex2f $dxmr $dyt
          glVertex2f $dxmr $dyb
        glEnd
      }
    }
  }

  source tcl/gui/application.tcl
  source tcl/gui/mode.tcl
  source tcl/gui/awidget.tcl
  source tcl/gui/container.tcl
  source tcl/gui/frame.tcl
  source tcl/gui/comfy_container.tcl
  source tcl/gui/border_container.tcl
  source tcl/gui/divided_container.tcl
  source tcl/gui/vertical_container.tcl
  source tcl/gui/horizontal_container.tcl
  source tcl/gui/stack_container.tcl
  source tcl/gui/label.tcl
  source tcl/gui/multi_line_label.tcl
  source tcl/gui/activator_label.tcl
  source tcl/gui/button.tcl
  source tcl/gui/tab_panel.tcl
  source tcl/gui/checkbox_radio.tcl
  source tcl/gui/slider.tcl
  source tcl/gui/spider_graph.tcl
  source tcl/gui/text_field.tcl
  source tcl/gui/scrollbar.tcl
  source tcl/gui/list.tcl
  source tcl/gui/progress_bar.tcl
  source tcl/gui/square_icon.tcl

  ## BEGIN: Abendstern-specific components

  source tcl/gui/ab/ship_spider_graph.tcl
  source tcl/gui/ab/simple_ship_display.tcl
  source tcl/gui/ab/ship_chooser.tcl
  source tcl/gui/ab/hangar_editor.tcl
}
package provide gui $gui::version
