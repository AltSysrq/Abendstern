  # The ShipSpiderGraph is a common component for displaying
  # a ship's abilities.
  class ShipSpiderGraph {
    inherit SpiderGraph

    #SpiderData
    variable sdAccel ;#Acceleration
    variable sdRota  ;#Rotational acceleration
    variable sdPower ;#Total power
    variable sdFPow  ;#Free power
    variable sdCapac ;#Capacitance
    variable sdArmour;#Armour (reinforcement)
    variable sdMass  ;#Mass

    constructor {} {
      set sdAccel  [new ::gui::SpiderDatum [_ A ship_chooser acceleration] 4.5e-7]
      set sdRota   [new ::gui::SpiderDatum [_ A ship_chooser rotation] 3.5e-6]
      set sdPower  [new ::gui::SpiderDatum [_ A ship_chooser power] 15.0]
      set sdFPow   [new ::gui::SpiderDatum [_ A ship_chooser free_power] 7.5]
      set sdCapac  [new ::gui::SpiderDatum [_ A ship_chooser capacitance] 10000.0]
      set sdArmour [new ::gui::SpiderDatum [_ A ship_chooser reinforcement] 5.0]
      set sdMass   [new ::gui::SpiderDatum [_ A ship_chooser mass] 16384.0]
      SpiderGraph::constructor [list \
        $sdAccel $sdRota   $sdPower  $sdFPow \
        $sdCapac $sdArmour $sdMass]
    } {
    }

    method setShip ship {
      $sdAccel  configure -value [$ship getAcceleration]
      $sdRota   configure -value [$ship getRotationAccel]
      $sdPower  configure -value [expr {[$ship getPowerSupply]/double([$ship cellCount])}]
      $sdFPow   configure -value [expr {max(0,
        ([$ship getPowerSupply]-[$ship getPowerDrain])/double([$ship cellCount]))}]
      $sdCapac  configure -value [expr {[$ship getMaximumCapacitance]/double([$ship cellCount])}]
      #$sdFireP configure -value [...]
      $sdArmour configure -value [$ship getReinforcement]
      $sdMass   configure -value [$ship getMass]
    }
  }
