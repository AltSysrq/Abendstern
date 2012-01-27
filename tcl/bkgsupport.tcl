# Handles incomming messages from the background thread
# Requires update to be called

proc dequeueIncommingBackgroundMessages {} {
  while {0 != [string length [set msg [bkg_get]]]} {
    namespace eval :: $msg
  }

  after 5 dequeueIncommingBackgroundMessages
}

bkg_start
after idle dequeueIncommingBackgroundMessages
