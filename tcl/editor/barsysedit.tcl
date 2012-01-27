# Root entry for system type chooser
set panel [new ::gui::VerticalContainer 0.01]
$panel add [new ::gui::Label "System Category" left]
$panel add [new ::gui::Button [_ A editor no_change] \
            "$ sets edit.sys${::systemToEdit}_type none; $this setbar syspaint"]
$panel add [new ::gui::Button [_ A sys power  ] "$this setbar syspow"]
$panel add [new ::gui::Button [_ A sys engine ] "$this setbar syseng"]
$panel add [new ::gui::Button [_ A sys weapon ] "$this setbar syswpn"]
$panel add [new ::gui::Button [_ A sys shield ] "$this setbar sysshl"]
$panel add [new ::gui::Button [_ A sys stealth] "$this setbar sysstl"]
$panel add [new ::gui::Button [_ A sys misc   ] "$this setbar sysmsc"]
$panel add [new ::gui::Label ""]
$panel add [new ::gui::Button "\a\[(danger)[_ A editor delete]\a\]" \
            "$ sets edit.sys${::systemToEdit}_type delete; $this setbar syspaint"]
$panel add [new ::gui::Label ""]
set cancelButton [new ::gui::Button [_ A gui cancel] "$this setbar syspaint"]
$cancelButton setCancel
$panel add $cancelButton

$ sets edit.current_mode none
