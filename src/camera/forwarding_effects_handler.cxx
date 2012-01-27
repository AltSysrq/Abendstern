/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/camera/forwarding_effects_handler.hxx
 */

#include "forwarding_effects_handler.hxx"

ForwardingEffectsHandler::ForwardingEffectsHandler(Ship* s) : ship(s) {}
void ForwardingEffectsHandler::impact(float amt) noth {
  ship->getField()->effects->impact(amt);
}
void ForwardingEffectsHandler::explode(Explosion* e) noth {
  ship->getField()->effects->explode(e);
}
