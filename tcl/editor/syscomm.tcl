# This file is used by the various system bars
# since they all share the same basic form.
# The local variable systems must be set appropriately.


set panel [new ::gui::VerticalContainer 0.01]

switch [$ str edit.ship_class] {
  A {set shipClass 2}
  B {set shipClass 1}
  C {set shipClass 0}
}

foreach {cname dname cls} $systems {
  if {$cls <= $shipClass} {
    $panel add [new ::gui::Button [_ A sys $dname] \
                "$ sets edit.sys${::systemToEdit}_type $cname; $this setbar syspaint"]
  }
}

$panel add [new ::gui::Label ""]
set cancelButton [new ::gui::Button [_ A gui cancel] "$this setbar syspaint"]
$cancelButton setCancel
$panel add $cancelButton
