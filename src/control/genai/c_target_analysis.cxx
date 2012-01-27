/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/c_target_analysis.hxx
 */

/*
 * c_target_analysis.cxx
 *
 *  Created on: 02.11.2011
 *      Author: jason
 */

#include <cmath>
#include <cstdlib>
#include <typeinfo>

#include "src/ship/everything.hxx"
#include "c_target_analysis.hxx"

using namespace std;

static TargetAnalysisCortex::input_map tacim;

TargetAnalysisCortex::TargetAnalysisCortex(const libconfig::Setting& species, Ship* s)
: Cortex(species, "target_analysis", cip_last, numOutputs, tacim),
  ship(s), previousShip(NULL), previousBestScore(0)
{
  #define CO(out) compileOutput(#out, (unsigned)out)
  CO(base);
  CO(power);
  CO(capac);
  CO(engine);
  CO(weapon);
  CO(shield);
  #undef CO
}

Cell* TargetAnalysisCortex::evaluate(float et, Ship* s) {
  if (s != previousShip) {
    previousShip = s;
    previousBestScore = 0;
  } else {
    previousBestScore *= pow(0.5f, et/8192.0f);
  }

  //Pick random Cell
  Cell* cell = s->cells[rand() % s->cells.size()];
  //Bail out if empty
  if (cell->isEmpty) return NULL;

  bindInputs(&inputs[0]);
  getCellExInputs(cell, &inputs[0]);
  float score = eval((unsigned)base);
  for (unsigned s=0; s<2; ++s) if (cell->systems[s]) {
    const type_info& ti = typeid(*cell->systems[s]);
    #define S(type,out,clazz) \
      else if (ti == typeid(type)) { \
        inputs[cip_sysclass] = clazz; \
        score += eval((unsigned)out); }
    if (false);
    S(AntimatterPower,          power,  3)
    S(DispersionShield,         shield, 3)
    S(GatlingPlasmaBurstLauncher,weapon, 3)
    S(MissileLauncher,          weapon, 3)
    S(MonophasicEnergyEmitter,  weapon, 3)
    S(ParticleBeamLauncher,     weapon, 3)
    S(RelIonAccelerator,        engine, 3)
    S(BussardRamjet,            engine, 2)
    S(FusionPower,              power,  2)
    S(MiniGravwaveDriveMKII,    engine, 2)
    S(PlasmaBurstLauncher,      weapon, 2)
    S(SemiguidedBombLauncher,   weapon, 2)
    S(ShieldGenerator,          shield, 2)
    S(SuperParticleAccelerator, engine, 2)
    S(Capacitor,                capac,  1)
    S(EnergyChargeLauncher,     weapon, 1)
    S(FissionPower,             power,  1)
    S(MagnetoBombLauncher,      weapon, 1)
    S(MiniGravwaveDrive,        engine, 1)
    S(ParticleAccelerator,      engine, 1)
    S(PowerCell,                power,  1)
    S(ReinforcementBulkhead,    shield, 1)
    #undef S
  }

  if (score > previousBestScore) {
    previousBestScore = score;
    return cell;
  } else return NULL;
}

float TargetAnalysisCortex::getScore() const {
  return ship->score;
}
