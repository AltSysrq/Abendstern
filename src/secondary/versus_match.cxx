#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include <vector>
#include <string>
#include <exception>
#include <stdexcept>

#include <libconfig.h++>

#include "versus_match.hxx"
#include "src/sim/game_field.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/shipio.hxx"
#include "src/secondary/confreg.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/globals.hxx"

using namespace std;

#define FIELD_SIZE 3
#define MS_PER_STEP 20
#define MAX_STEPS (60*1000/MS_PER_STEP)

//Insignias are global, so use something outside of the range touched by normal
//games
#define TESTING_INSIGNIA 0x8001
#define AGAINST_INSIGNIA 0x8002

static set<VersusMatch*> currentInstances;

VersusMatch::VersusMatch(const string& testroot,
                         unsigned testCount,
                         const string& againstroot,
                         unsigned againstCount)
throw (runtime_error)
: field(FIELD_SIZE, FIELD_SIZE),
  currentStep(0)
{
  try {
    for (unsigned i = 0; i < testCount; ++i) {
      Ship* s = loadShip(&field, testroot.c_str());
      field.add(s);
      testing.push_back(s);
      s->insignia = TESTING_INSIGNIA;
      s->controller = new AIControl(s, conf["AI"]["test"]);
      s->shipExistenceFailure = &notifyShipDeath;
    }

    for (unsigned i = 0; i < againstCount; ++i) {
      Ship* s = loadShip(&field, againstroot.c_str());
      field.add(s);
      testing.push_back(s);
      s->insignia = TESTING_INSIGNIA;
      s->controller = new AIControl(s, conf["AI"]["test"]);
      s->shipExistenceFailure = &notifyShipDeath;
    }
  } catch (libconfig::ConfigException& ce) {
    throw runtime_error(ce.what());
  }

  currentInstances.insert(this);
}

VersusMatch::~VersusMatch() {
  currentInstances.erase(this);
}

bool VersusMatch::step() noth {
  //Check for timeout
  if (currentStep >= MAX_STEPS)
    return false;

  //Check for one team having been eliminated
  bool anyAlive = false;
  for (unsigned i = 0; i < testing.size() && !anyAlive; ++i)
    anyAlive |= !!testing[i];
  for (unsigned i = 0; i < against.size() && !anyAlive; ++i)
    anyAlive |= !!against[i];

  if (!anyAlive)
    return false;

  //Still more to do
  ++currentStep;
  field.update(MS_PER_STEP);
  return true;
}

float VersusMatch::score() const noth {
  float s = 0;
  for (unsigned i = 0; i < testing.size(); ++i)
    if (!testing[i])
      //Own team member got killed, reduce score
      s -= 1.0f / testing.size();
  for (unsigned i = 0; i < against.size(); ++i)
    if (!against[i])
      //Enemy got killed, increase score
      s += 1.0f / against.size();

  return s;
}

void VersusMatch::notifyShipDeath(Ship* s, bool) {
  for (set<VersusMatch*>::const_iterator it = currentInstances.begin();
       it != currentInstances.end(); ++it)
    (*it)->shipDeath(s);

  s->shipExistenceFailure = NULL;
}

void VersusMatch::shipDeath(Ship* s) noth {
  for (unsigned i = 0; i < testing.size(); ++i) {
    if (testing[i] == s) {
      testing[i] = NULL;
      return;
    }
  }
  for (unsigned i = 0; i < against.size(); ++i) {
    if (against[i] == s) {
      against[i] = NULL;
      return;
    }
  }
}
