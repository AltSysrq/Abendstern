# Loads secondary packages not directly required by BootManager

if {$PLATFORM == "WINDOWS"} {
  source tcllib/aes/aes.tcl
  source tcllib/sha1/sha256.tcl
  source tcllib/uuid/uuid.tcl
}

package require http
package require tls
package require aes
package require sha256
package require uuid

source tcl/lssinfo.tcl
