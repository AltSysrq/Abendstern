/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Implementation of the AIM_CheckHasWeapons AI module
 */

#include "aim_check_targetless.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/ship/ship.hxx"

AIM_CheckTargetless::AIM_CheckTargetless(AIControl* c,
                                         const libconfig::Setting& s)
: AIModule(c),
  targetless(s.exists("targetless")? (const char*)s["targetless"] : ""),
  otherwise(s.exists("otherwise")? (const char*)s["otherwise"] : "")
{ }

void AIM_CheckTargetless::action() {
  if (!ship->target.ref || !((Ship*)ship->target.ref)->hasPower())
    controller.setState(targetless.c_str());
  else
    controller.setState(otherwise.c_str());
}

static AIModuleRegistrar<AIM_CheckTargetless>
registrar("target/check_targetless");
