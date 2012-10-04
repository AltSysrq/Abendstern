# The Ship Properties Cache (spc) provides access to commonly-needed ship
# properties by transparently using a cache to avoid triggering the loading of
# ship files. That is, the ship config itself will only ever be accessed if the
# properties do not exist in the cache OR the ship file is already loaded.
#
# The cache is a libconfig file mounted to spc and located at
#   [homeq spc.dat]
# The top-level consists of keys named uXXX, where XXX is the part of the ship
# mount name before the / (eg, 1 or local). Each of those keys references
# another group containing keys named according to the filename of the ship
# (without the .ship); this works since the filename translator ensures that
# all filenames are alphanumeric only and never begin with a digit.
#
# Each of these groups consists merely of a list of string key/value pairs,
# which are currently:
#   name, author, class, category, guid, ownerid
namespace eval spc {
  set PROPERTIES [list name author class category guid ownerid]

  # Set (dict) of ship mounts which have already been refreshed in the cache
  # (so further hits against the ship will not recopy the data over to cache).
  set refreshedMounts {}

  proc mount2loc mount {
    set mount [shipMount2Name $mount]
    return "spc.u[string map {/ .} $mount]"
  }

  proc mount2uentry mount {
    set mount [shipMount2Name $mount]
    return "u[string range $mount 0 [string first / $mount]-1]"
  }

  proc ensure-user-exists mount {
    set ul [mount2uentry $mount]
    if {![$ exists spc.$ul]} {
      $ add spc $ul STGroup
    }
  }

  proc prepare-cache-ship mount {
    set loc [mount2loc $mount]
    catch {
      $ remove $loc
    }
    ensure-user-exists $mount
    $ add spc.[mount2uentry $mount] \
        [string range $mount [string first / $mount]+1 end] \
        STGroup
  }

  proc use-cache-for {mount property} {
    expr {![$ loaded $mount] && [$ exists [mount2loc $mount].$property]}
  }

  proc should-update-cache mount {
    expr {![dict exists $::spc::refreshedMounts $mount]}
  }

  proc update-cache mount {
    prepare-cache-ship $mount
    set loc [mount2loc $mount]
    foreach prop $::spc::PROPERTIES {
      $ adds $loc $prop [get-property-from-ship $prop $mount]
    }

    dict set ::spc::refreshedMounts $mount {}
  }

  proc get-property-from-ship {prop mount} {
    switch -exact -- $prop {
      ownerid { set type int }
      default { set type str }
    }
    switch -exact -- $prop {
      category {
        return [Ship_categorise $mount]
      }
      ownerid -
      guid {
        # Not guaranteed to exist
        if {[$ exists $mount.info.$prop]} {
          return [$ $type $mount.info.$prop]
        } else {
          return ""
        }
      }
      default {
        return [$ $type $mount.info.$prop]
      }
    }
  }

  # Returns the given property from the given ship.
  # If something goes wrong, returns an empty string. This behaviour is to
  # ensure that code which assumes the accesses always work is correct, and
  # because ship errors should be detected when the ship is actually to be
  # loaded; handling these problems everywhere that merely needs to know about
  # the ships would be redundant and cumbersome.
  proc get {mount property} {
    if {[use-cache-for $mount $property]} {
      return [$ str [mount2loc $mount].$property]
    } else {
      if {[catch {
        if {[should-update-cache $mount]} {
          update-cache $mount
        }

        set result [get-property-from-ship $property $mount]
      } err]} {
        log "Could not get $property of $mount: $err"
        return ""
      }
      return $result
    }
  }
}

# On init, load the cache, or create it if that fails
if {[catch {
  $ open [homeq spc.dat] spc
}]} {
  $ create [homeq spc.dat] spc
}
