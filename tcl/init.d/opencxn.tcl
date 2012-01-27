# Opens a connection to the Abendstern Network

::abnet::openConnection

$state setCallback [_ A boot opencxn] {
  if {$::abnet::busy} {
    return [expr {min(100,[::abnet::timeSinceResponse]*100/$::abnet::MAXIMUM_BUSY_TIME)}]
  }
  return 200
}
