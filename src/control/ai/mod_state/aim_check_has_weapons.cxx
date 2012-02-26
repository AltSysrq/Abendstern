/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Implementation of the AIM_CheckHasWeapons AI module
 */

#include "aim_check_has_weapons.hxx"
#include "src/control/ai/aictrl.hxx"

AIM_CheckHasWeapons::AIM_CheckHasWeapons(AIControl* c,
                                         const libconfig::Setting& s)
: AIModule(c),
  unarmed(s.exists("unarmed")? (const char*)s["unarmed"] : ""),
  otherwise(s.exists("otherwise")? (const char*)s["otherwise"] : "")
{ }

void AIM_CheckHasWeapons::action() {
  if (controller.gglob("ship_has_weapons", true))
    controller.setState(otherwise.c_str());
  else
    controller.setState(unarmed.c_str());
}

static AIModuleRegistrar<AIM_CheckHasWeapons>
registrar("state/check_has_weapons");
