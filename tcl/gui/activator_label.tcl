  # This class was originally slider-specific (as SliderActivatorLabel),
  # but has since been generalized to work with anything with a gainFocus method
  class ActivatorLabel {
    inherit Label
    variable sliderImpl

    constructor {name impl} {
      Label::constructor $name left
    } {
      set sliderImpl $impl
    }

    method setAccelerator p {
      # Bypass Label's deactivation
      AWidget::setAccelerator $p
    }

    method action {} {
      $sliderImpl gainFocus
    }

    # Prevent clicking from focusing
    method button {evt x y} {}

    method draw {} {
      # If focussed, become all white
      if {[$sliderImpl isFocused]} {
        set old $displayName
        set displayName "\a\[(white)$old\a\]"
        Label::draw
        set displayName $old
      } else { Label::draw }
    }
  }
