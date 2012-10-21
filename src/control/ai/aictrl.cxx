/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/aictrl.hxx
 */

/*
 * aictrl.cxx
 *
 *  Created on: 16.02.2011
 *      Author: jason
 */

#include <algorithm>
#include <map>
#include <cmath>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <exception>
#include <stdexcept>

#include <libconfig.h++>

#include "aictrl.hxx"
#include "aimod.hxx"
#include "src/secondary/confreg.hxx"
#include "src/globals.hxx"
#include "src/ship/ship.hxx"
#include "src/tcl_iface/bridge.hxx"

using namespace std;
using namespace libconfig;

#define TIME_BETWEEN_ACTIONS 50.0f

/* Map of strings to module factories. This is created the first
 * time a module is registered.
 */
static map<string, AIControl::module_factory>* factories;

void AIControl::init(const Setting& conf) {
  if (iaicInfo)
    iaicInfoPrefix = "iaic." + conf.getPath();

  //Load the states
  for (unsigned i=0; i<conf.getLength(); ++i) {
    const Setting& state(conf[i]);
    const char* stateName = state.getName();
    State s;
    s.totalWeight=0;

    for (unsigned j=0; j<state.getLength(); ++j) {
      const char* namesrc;
      bool isModule;
      if (state[j].exists("module")) {
        isModule = true;
        namesrc = "module";
      } else {
        isModule = false;
        namesrc = "reflex";
      }
      const char* modname=state[j][namesrc];
      //Explicitly initialise weight to suppress warning since G++ can't tell
      //that it simply isn't touched if !isModule.
      int weight = 0;
      if (isModule) {
        weight = state[j]["weight"];
        if (weight <= 0)
          throw runtime_error("Negative weight for module");
      }
      AIModule* module = createAIModule(modname, state[j]);
      if (!module)
        throw runtime_error("Unrecognised module name");
      if (isModule) {
        Module mod = { module, (unsigned)weight };
        s.modules.push_back(mod);
        s.totalWeight += weight;
      } else {
        if (s.reflexes.size() == 4)
          throw runtime_error("State has more than 4 reflexes");
        s.reflexes.push_back(module);
      }
    }

    if (s.modules.empty())
      throw runtime_error("Empty state");

    states[string(stateName)] = s;
  }

  if (!states.size())
    throw runtime_error("Empty AI information");

  setState("boot");
  if (!currentState)
    throw runtime_error("Missing boot state");
}

AIControl::AIControl(Ship* s, const libconfig::Setting& root, libconfig::Setting* iaic)
: Controller(s),
  iaicInfo(iaic), currentState(NULL),
  targetTheta(s->getRotation()), currentWeapon(0),
  timeSinceLastAction(0)
{
  init(root);
}

AIControl::AIControl(Ship* s, const char* root, const char* iaic)
: Controller(s),
  currentState(NULL),
  targetTheta(s->getRotation()), currentWeapon(0),
  timeSinceLastAction(0)
{
  //If non-NULL and non-empty, set the IAIC root
  if (iaic && *iaic)
    iaicInfo = &conf.lookup(iaic);

  init(conf.lookup(root));
}

AIControl::~AIControl() {
  for (map<string,State>::const_iterator it=states.begin();
       it != states.end(); ++it) {
    for (unsigned i=0; i<(*it).second.modules.size(); ++i)
      delete (*it).second.modules[i].module;
    for (unsigned i = 0; i < it->second.reflexes.size(); ++i)
      delete it->second.reflexes[i];
  }
}

void AIControl::update(float et) noth {
  float theta = ship->getRotation();
  float vtheta = ship->getVRotation();
  //Skip rotation calculation (and an FPE) if we can't rotate
  if (0 != ship->getRotationAccel()) {
    float thetaDiff = theta - targetTheta;
    while (thetaDiff < -pi) thetaDiff += 2*pi;
    while (thetaDiff > +pi) thetaDiff -= 2*pi;
    //The AI can't handle ships that spin too quickly
    //The threshhold was determined emperically.
    float rotMul = 1;// min(1.0f, 600.0f/(ship->getRotationAccel() / pi * 180 * 1000000));
    //Near-infinite time to match if turning the wrong way, or not turning at all.
    float timeToMatch = (vtheta != 0? thetaDiff / vtheta < 0? -thetaDiff / vtheta
                                   : HUGE_VAL : HUGE_VAL);
    float timeToStop = fabs(rotMul * vtheta / ship->getRotationAccel());
    //Invert sign if we will overshoot it
    if (timeToMatch < timeToStop) {
      thetaDiff = -thetaDiff;
      rotMul = 1;
    }
    if (fabs(thetaDiff) > 0)
      ship->spin((thetaDiff < 0? +1 : -1)*et*rotMul*STD_ROT_RATE*
                   (timeToStop*2 > timeToMatch? timeToMatch/timeToStop/2 : 1));
  }

  /* Both loops below operate on data structures that may
   * be modified during execution. However, the loops are coded
   * so that unexpected removals have no effect other than possibly
   * skipping the next module (in the rare case that they remove
   * a DIFFERENT module).
   */

  //Handle timers
  for (unsigned i=0; i<timers.size(); ++i) {
    timers[i].timeLeft -= et;
    AIModule* mod = timers[i].module;
    mod->timer(timers[i].arg);
    //See if it removed itself or something earlier
    if (timers.size() <= i || timers[i].module != mod) {
      //We shouldn't increment
      --i;
      continue;
    }

    if (timers[i].timeLeft <= 0)
      timers.erase(timers.begin() + (i--));
  }

  //Handle interrupts
  for (unsigned i=0; i<interrupts.size(); ++i) {
    Interrupt& in = interrupts[i];
    //See if values are the same
    bool same=(
       (in.src->getType() == Setting::TypeBoolean
     && in.oldValue.type == Variable::Bool
     && in.oldValue.asBool == in.src->operator bool())

    ||  ((in.src->getType() == Setting::TypeInt || in.src->getType() == Setting::TypeInt64)
     && in.oldValue.type == Variable::Int
     && in.oldValue.asInt == in.src->operator int())

    ||  (in.src->getType() == Setting::TypeFloat
     && in.oldValue.type == Variable::Float
     && in.oldValue.asFloat == in.src->operator float())

    ||  (in.src->getType() == Setting::TypeString
     && in.oldValue.type == Variable::String
     && in.oldValue.asString == in.src->operator const char*())
    );

    if (!same) {
      //Value changed, call module
      AIModule* mod=in.listener;
      mod->iaic(in.arg);

      //Cancel increment and continue loop if we
      //no longer know where this module is
      if (i >= interrupts.size() || interrupts[i].listener != mod) {
        --i;
        continue;
      }

      //Update value
      in.oldValue.asObject.assign(NULL);
      in.oldValue.asString.clear();

      switch (in.src->getType()) {
        case Setting::TypeBoolean:
          in.oldValue.type = Variable::Bool;
          in.oldValue.asBool = *in.src;
          break;
        case Setting::TypeInt:
        case Setting::TypeInt64:
          in.oldValue.type = Variable::Int;
          in.oldValue.asInt = *in.src;
          break;
        case Setting::TypeFloat:
          in.oldValue.type = Variable::Float;
          in.oldValue.asFloat = *in.src;
          break;
        case Setting::TypeString:
          in.oldValue.type = Variable::String;
          in.oldValue.asString = (const char*)*in.src;
          break;
        default: /* Silently ignore */;
      }
    }
  }

  timeSinceLastAction += et;
  while (timeSinceLastAction >= TIME_BETWEEN_ACTIONS) {
    //Run reflexes
    for (unsigned i = 0; i < currentState->reflexes.size(); ++i)
      currentState->reflexes[i]->action();

    //If there is currently a proceedure, run that
    if (!currentProcedure.empty()) {
      currentProcedure.front()->action();
      currentProcedure.pop();
    } else {
      //Select randomly
      unsigned r = rand() % currentState->totalWeight;
      AIModule* mod = NULL;
      for  (unsigned i=0; !mod; ++i)
        if (r < currentState->modules[i].weight)
          mod = currentState->modules[i].module;
        else
          r -= currentState->modules[i].weight;

      assert(mod);
      mod->action();
    }

    timeSinceLastAction -= TIME_BETWEEN_ACTIONS;
  }
}

AIControl::Variable& AIControl::operator[](const char* key) {
  string skey(key);
  varmap::iterator it=globalVariables.find(skey);
  if (it == globalVariables.end()) {
    Variable inv;
    inv.type = Variable::Invalid;
    return (globalVariables[skey] = inv);
  } else return (*it).second;
}

/* It's too bad we couldn't use templates for these redundant functions. */
bool AIControl::gglob(const char* key, bool def) {
  Variable& var((*this)[key]);
  if (var.type != Variable::Bool) {
    var.asObject.assign(NULL);
    var.asString.clear();
    var.type = Variable::Bool;
    var.asBool = def;
  }
  return var.asBool;
}

int AIControl::gglob(const char* key, int def) {
  Variable& var((*this)[key]);
  if (var.type == Variable::Float) return (int)var.asFloat;
  if (var.type != Variable::Int) {
    var.asObject.assign(NULL);
    var.asString.clear();
    var.type = Variable::Int;
    var.asInt = def;
  }
  return var.asInt;
}

float AIControl::gglob(const char* key, float def) {
  Variable& var((*this)[key]);
  if (var.type == Variable::Int) return (float)var.asInt;
  if (var.type != Variable::Float) {
    var.asObject.assign(NULL);
    var.asString.clear();
    var.type = Variable::Float;
    var.asFloat = def;
  }
  return var.asFloat;
}

const char* AIControl::gglob(const char* key, const char* def) {
  Variable& var((*this)[key]);
  if (var.type != Variable::String) {
    var.asObject.assign(NULL);
    var.type = Variable::String;
    var.asString = def;
  }
  return var.asString.c_str();
}

GameObject* AIControl::gglob(const char* key, GameObject* def) {
  Variable& var((*this)[key]);
  if (var.type != Variable::Object) {
    var.asString.clear();
    var.type = Variable::Object;
    var.asObject.assign(def);
  }
  return var.asObject.ref;
}

void AIControl::sglob(const char* key, bool val) {
  Variable& var((*this)[key]);
  if (var.type != Variable::Bool) {
    var.asString.clear();
    var.asObject.assign(NULL);
    var.type = Variable::Bool;
  }
  var.asBool = val;
}

void AIControl::sglob(const char* key, int val) {
  Variable& var((*this)[key]);
  if (var.type != Variable::Int) {
    var.asString.clear();
    var.asObject.assign(NULL);
    var.type = Variable::Int;
  }
  var.asInt = val;
}

void AIControl::sglob(const char* key, float val) {
  Variable& var((*this)[key]);
  if (var.type != Variable::Float) {
    var.asString.clear();
    var.asObject.assign(NULL);
    var.type = Variable::Float;
  }
  var.asFloat=val;
}

void AIControl::sglob(const char* key, const char* val) {
  Variable& var((*this)[key]);
  if (var.type != Variable::String) {
    var.asObject.assign(NULL);
    var.type = Variable::String;
  }
  var.asString=val;
}

void AIControl::sglob(const char* key, GameObject* val) {
  Variable& var((*this)[key]);
  if (var.type != Variable::Object) {
    var.asString.clear();
    var.type = Variable::Object;
  }
  var.asObject.assign(val);
}

AIControl::Variable& AIControl::operator()(const char* key) {
  string skey(key);
  varmap::iterator it=perStateVariables.find(skey);
  if (it == perStateVariables.end()) {
    Variable inv;
    inv.type = Variable::Invalid;
    return (perStateVariables[skey] = inv);
  } else return (*it).second;
}

bool AIControl::gstat(const char* key, bool def) {
  Variable& var((*this)(key));
  if (var.type != Variable::Bool) {
    var.asObject.assign(NULL);
    var.asString.clear();
    var.type = Variable::Bool;
    var.asBool = def;
  }
  return var.asBool;
}

int AIControl::gstat(const char* key, int def) {
  Variable& var((*this)(key));
  if (var.type == Variable::Float) return (int)var.asFloat;
  if (var.type != Variable::Int) {
    var.asObject.assign(NULL);
    var.asString.clear();
    var.type = Variable::Int;
    var.asInt = def;
  }
  return var.asInt;
}

float AIControl::gstat(const char* key, float def) {
  Variable& var((*this)(key));
  if (var.type == Variable::Int) return (float)var.asInt;
  if (var.type != Variable::Float) {
    var.asObject.assign(NULL);
    var.asString.clear();
    var.type = Variable::Float;
    var.asFloat = def;
  }
  return var.asFloat;
}

const char* AIControl::gstat(const char* key, const char* def) {
  Variable& var((*this)(key));
  if (var.type != Variable::String) {
    var.asObject.assign(NULL);
    var.type = Variable::String;
    var.asString = def;
  }
  return var.asString.c_str();
}

GameObject* AIControl::gstat(const char* key, GameObject* def) {
  Variable& var((*this)(key));
  if (var.type != Variable::Object) {
    var.asString.clear();
    var.type = Variable::Object;
    var.asObject.assign(def);
  }
  return var.asObject.ref;
}

void AIControl::sstat(const char* key, bool val) {
  Variable& var((*this)(key));
  if (var.type != Variable::Bool) {
    var.asString.clear();
    var.asObject.assign(NULL);
    var.type = Variable::Bool;
  }
  var.asBool = val;
}

void AIControl::sstat(const char* key, int val) {
  Variable& var((*this)(key));
  if (var.type != Variable::Int) {
    var.asString.clear();
    var.asObject.assign(NULL);
    var.type = Variable::Int;
  }
  var.asInt = val;
}

void AIControl::sstat(const char* key, float val) {
  Variable& var((*this)(key));
  if (var.type != Variable::Float) {
    var.asString.clear();
    var.asObject.assign(NULL);
    var.type = Variable::Float;
  }
  var.asFloat=val;
}

void AIControl::sstat(const char* key, const char* val) {
  Variable& var((*this)(key));
  if (var.type != Variable::String) {
    var.asObject.assign(NULL);
    var.type = Variable::String;
  }
  var.asString=val;
}

void AIControl::sstat(const char* key, GameObject* val) {
  Variable& var((*this)(key));
  if (var.type != Variable::Object) {
    var.asString.clear();
    var.type = Variable::Object;
  }
  var.asObject.assign(val);
}

AIControl::WeaponInfo& AIControl::getWeaponInfo(int w) {
  assert(w >= 0 && w < (int)numWeapons);
  return weaponInfos[w];
}

AIControl::SingleWeaponInfo& AIControl::getWeaponInfo(int w, unsigned ix) {
  assert(w >= 0 && w < (int)numWeapons);
  assert(ix < weaponInfos[w].size());
  return weaponInfos[w][ix];
}

void AIControl::setState(const char* stateName) {
  map<string,State>::iterator it = states.find(string(stateName));
  if (it == states.end()) return;
  if (&(*it).second == currentState) return;

  if (currentState) {
    for (unsigned i=0; i<currentState->modules.size(); ++i)
      currentState->modules[i].module->deactivate();
  }

  perStateVariables.clear();
  timers.clear();
  interrupts.clear();

  currentState = &(*it).second;
  for (unsigned i=0; i<currentState->modules.size(); ++i)
    currentState->modules[i].module->activate();
}

void AIControl::addTimer(AIModule* mod, float time, int arg) {
  if (!mod) return;

  ModuleTimer timer = { mod, time, arg };
  timers.push_back(timer);
}

void AIControl::delTimer(AIModule* mod, int arg) {
  for (unsigned i=0; i<timers.size(); ++i)
    if (timers[i].module == mod && timers[i].arg == arg) {
      timers.erase(timers.begin() + i);
      return;
    }
}

void AIControl::addInterrupt(AIModule* mod, const char* path, int arg) {
  if (!mod) return;

  try {
    Setting& src(conf.lookup(path));
    Variable val;
    switch (src.getType()) {
      case Setting::TypeBoolean:
        val.type = Variable::Bool;
        val.asBool = src;
        break;
      case Setting::TypeInt:
      case Setting::TypeInt64:
        val.type = Variable::Int;
        val.asInt = src;
        break;
      case Setting::TypeFloat:
        val.type = Variable::Float;
        val.asFloat = src;
        break;
      case Setting::TypeString:
        val.type = Variable::String;
        val.asString = (const char*)src;
        break;
      default:
        return;
    }
    Interrupt in = { &src, val, mod, arg };
    interrupts.push_back(in);
  } catch (...) {
  }
}

void AIControl::delInterrupt(AIModule* mod, int arg) {
  for (unsigned i=0; i<interrupts.size(); ++i)
    if (interrupts[i].listener == mod && interrupts[i].arg == arg) {
      interrupts.erase(interrupts.begin() + i);
      return;
    }
}

void AIControl::appendProcedure(AIModule* mod) {
  currentProcedure.push(mod);
}

float AIControl::getTargetTheta() const {
  return targetTheta;
}

void AIControl::setTargetTheta(float f) {
  if (f < 0) {
    unsigned mul = 1 + (unsigned)((-f)/2/pi);
    f += 2*pi*mul;
  }
  targetTheta = fmod(f, 2*pi);
}

int AIControl::getCurrentWeapon() const {
  return currentWeapon;
}

void AIControl::setCurrentWeapon(int w) {
  if (w < 0 || w >= (int)numWeapons) return;
  currentWeapon=w;
}

bool AIControl::hasIAIC() const {
  return iaicInfo != NULL;
}

Setting& AIControl::getIAIC() {
  return *iaicInfo;
}

const char* AIControl::getIAICPrefix() const {
  return iaicInfoPrefix.c_str();
}

AIModule* AIControl::createAIModule(const char* name, const libconfig::Setting& conf) {
  map<string, module_factory>::const_iterator it = factories->find(string(name));
  if (it == factories->end()) return NULL;
  else return (*it).second(this, conf);
}

void AIControl::registerAIModule(const char* name, module_factory factory) {
  assert(factory);

  //Create map if not yet created
  static bool hasCreatedMap = false;
  if (!hasCreatedMap) {
    factories = new map<string, module_factory>;
    hasCreatedMap=true;
  }

  (*factories)[string(name)] = factory;
}
