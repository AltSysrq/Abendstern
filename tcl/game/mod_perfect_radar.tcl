# Mixin to BasicGame which sets perfectRadar to true on initialisation.
# Since it depends on variables in BasicGame, it must be inherited first
# (ie, inherit MixinPerfectRadar BasicGame).
class MixinPerfectRadar {
  constructor {} {
    [cget -field] configure -perfectRadar yes
  }
}
