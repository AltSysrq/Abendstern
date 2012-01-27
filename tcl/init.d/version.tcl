# Reads in the local version information.
# Depends on loadlib

set VERSION unknown
catch {
  set f [open version r]
  set VERSION [string trim [read $f]]
  close $f
  set VERSION [string range $v 0 [string length "20110102030405"]]
  httpa::config -useragent "Abendstern $VERSION"
}
