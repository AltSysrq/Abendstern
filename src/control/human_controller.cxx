/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/human_controller.hxx
 */

#include <SDL.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <typeinfo>
#include <algorithm>

#include <libconfig.h++>

#include "human_controller.hxx"
#include "hc_conf.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/insignia.hxx"
#include "src/ship/sys/ship_system.hxx"
#include "src/ship/auxobj/shield.hxx"
#include "src/globals.hxx"
#include "src/secondary/global_chat.hxx"
#include "src/audio/ui_sounds.hxx"
#include "src/exit_conditions.hxx"
using namespace std;
using namespace hc_conf;
using namespace libconfig;

#define REPEAT_T 200

static void saveEnergyLevels(Ship*) noth;

namespace action {
  bool buttonsPressed[SDL_BUTTON_X2+1];
  bool keysPressed[SDLK_LAST+1];
  //For auto-repeat
  float timeSinceMouseButtonRepeat[lenof(buttonsPressed)];
  float timeSinceKeyboardRepeat[lenof(keysPressed)];
  //See note in analogue_rotate
  float analogueRotationLastDSec;

  //Privately used to know how long the last frame was
  //Even though multiple HumanControllers will overwrite
  //this, it will be with the same value, so there is
  //no conflict.
  float lastFrameTime;

  //Copied from HumanController
  bool spunThisFrame;

  void accel_on(Ship* s, ActionDatum& datum) {
    if (s) s->setThrustOn(true);
  }
  void accel_off(Ship* s, ActionDatum& datum) {
    if (s) s->setThrustOn(false);
  }

  void decel_on(Ship* s, ActionDatum& datum) {
    if (s) s->setBrakeOn(true);
  }
  void decel_off(Ship* s, ActionDatum& datum) {
    if (s) s->setBrakeOn(false);
  }

  void turn_on(Ship* s, ActionDatum& datum) {
    if (spunThisFrame) return;
    spunThisFrame=true;
    if (s) s->spin(datum.amt*lastFrameTime);
  }

  void throttle_on(Ship* s, ActionDatum& datum) {
    if (s) s->setThrust(s->getTrueThrust()+datum.amt);
  }

  void set_current_weapon_on(Ship* s, ActionDatum& datum) {
    const char* name=(const char*)datum.pair.second.ptr;
    Weapon weap;
    if (0 == strcmp(name, "EnergyChargeLauncher")) weap=Weapon_EnergyCharge;
    else if (0 == strcmp(name, "MagnetoBombLauncher")) weap=Weapon_MagnetoBomb;
    else if (0 == strcmp(name, "PlasmaBurstLauncher")) weap=Weapon_PlasmaBurst;
    else if (0 == strcmp(name, "SGBombLauncher")) weap=Weapon_SGBomb;
    else if (0 == strcmp(name, "GatlingPlasmaBurstLauncher")) weap=Weapon_GatlingPlasma;
    else if (0 == strcmp(name, "MonophasicEnergyEmitter")) weap=Weapon_Monophase;
    else if (0 == strcmp(name, "MissileLauncher")) weap=Weapon_Missile;
    else if (0 == strcmp(name, "ParticleBeamLauncher")) weap=Weapon_ParticleBeam;
    else {
      cerr << "Request to switch to unknown weapon type: " << name << endl;
      exit(EXIT_MALFORMED_CONFIG);
    }
    ((HumanController*)datum.pair.first)->currentWeapon = weap;

    HumanController* hc = (HumanController*)datum.pair.first;
    if (hc->ship) {
      if (weapon_exists(hc->ship, weap)) switch (weap) {
        case Weapon_EnergyCharge:
          audio::root.add(new audio::Dtmf20);
          break;
        case Weapon_MagnetoBomb:
          audio::root.add(new audio::Dtmf10);
          break;
        case Weapon_PlasmaBurst:
          audio::root.add(new audio::Dtmf21);
          break;
        case Weapon_SGBomb:
          audio::root.add(new audio::Dtmf11);
          break;
        case Weapon_GatlingPlasma:
          audio::root.add(new audio::Dtmf22);
          break;
        case Weapon_Monophase:
          audio::root.add(new audio::Dtmf23);
          break;
        case Weapon_Missile:
          audio::root.add(new audio::Dtmf12);
          break;
        case Weapon_ParticleBeam:
          audio::root.add(new audio::Dtmf13);
          break;
      } else {
        audio::root.add(new audio::WeaponNotFound);
      }
    }
  }

  void adjust_weapon_power_on(Ship* s, ActionDatum& datum) {
    if (!s) return;
    Weapon currentWeapon=((HumanController*)datum.pair.first)->currentWeapon;
    for (long times=datum.pair.second.cnt; times; times>0? --times : ++times)
      if (times > 0)
        weapon_incEnergyLevel(s, currentWeapon);
      else
        weapon_decEnergyLevel(s, currentWeapon);
    saveEnergyLevels(s);
  }

  void fire_on(Ship* s, ActionDatum& datum) {
    if (s) weapon_fire(s, ((HumanController*)datum.ptr)->currentWeapon);
  }

  void retarget_on(Ship* s, ActionDatum& datum) {
    if (s) {
      ((HumanController*)datum.ptr)->retarget();
    }
  }

  void selfDestruct_on(Ship* s, ActionDatum& datum) {
    if (s) selfDestruct(s);
  }

  void stealth_on(Ship* s, ActionDatum&) {
    if (s) {
      if (s->isStealth())
        audio::root.add(new audio::StealthOff);
      else
        audio::root.add(new audio::StealthOn);
      s->toggleStealthMode();
    }
  }

  void compose_message_on(Ship*, ActionDatum& datum) {
    //HumanController* hc = ((HumanController*)datum.ptr);
    isCompositionBufferInUse=true;
    compositionBuffer[0]=0;
    compositionBufferIndex=0;
  }

  const DigitalAction accel = { {0}, false, false, accel_on, accel_off },
                      decel = { {0}, false, false, decel_on, decel_off },
                      turnLeft  = { {+STD_ROT_RATE}, true, true, turn_on, NULL },
                      turnRight = { {-STD_ROT_RATE}, true, true, turn_on, NULL },
                      accelMore = { {+0.05f}, false, false, throttle_on, NULL },
                      accelLess = { {-0.05f}, false, false, throttle_on, NULL },
                      stealth   = { {0}, false, false, stealth_on, NULL },
                      compose   = { {0}, false, false, compose_message_on, NULL };

  void analogue_rotate(Ship* ship, float amt, bool recentre) {
    if (spunThisFrame) return;
    if (!ship) return;
    spunThisFrame=true;
    /* OK, the reason the limit
     *   STD_ROT_RATE*lastFrameTime
     * doesn't work is that it assumes it is possible for
     * mouse events to arrive at the same rate as the framerate.
     * At high a high framerate, it canNOT.
     * We therefore keep track of our rotation magnitude for the
     * last second (stored in analogueRotationLastDSec), and ensure
     * that amt will not make it exceed STD_ROT_RATE*100.
     */
    if (recentre) {
      //Verify amount
      if (fabs(amt)+analogueRotationLastDSec > STD_ROT_RATE*100) {
        if (amt < 0) amt = analogueRotationLastDSec - STD_ROT_RATE*100;
        else         amt = STD_ROT_RATE*100 - analogueRotationLastDSec;
      }
      analogueRotationLastDSec += fabs(amt);
    } else {
      //Input freq has no effect in static mode
      if (fabs(amt) > STD_ROT_RATE) amt = (amt>0? STD_ROT_RATE : -STD_ROT_RATE);
      amt *= lastFrameTime;
    }
    ship->spin(-amt);
  }

  void analogue_throttle(Ship* ship, float amt, bool recentre) {
    if (!ship) return;
    float throttle=ship->getTrueThrust();
    if (recentre)
      throttle+=amt;
    else
      throttle = amt+0.5f; //0 is at centre
    if (throttle>1) throttle=1;
    if (throttle<0) throttle=0;
    ship->setThrust(throttle);
  }
  const AnalogueAction rotate = { AnalogueAction::Rotation, 1.0f, 0.5f, true, analogue_rotate },
                       throttle = { AnalogueAction::EnginePower, 0.5f, 0.5f, true, analogue_throttle },
                       noAction = { AnalogueAction::Rotation, 0, 0, true, NULL };
};

using action::timeSinceKeyboardRepeat;
using action::timeSinceMouseButtonRepeat;
using action::analogueRotationLastDSec;

/* Sets energy level on all weapons, as configured under
 * ship.info.energy_levels. All errors are silently ignored.
 */
static void setEnergyLevels(Ship* s) noth {
  if (!s) return;
  if (s->typeName.empty()) return;
  #define SET(name) try { \
    weapon_setEnergyLevel(s, Weapon_##name, conf[s->typeName.c_str()]["info"]["energy_levels"][#name]);\
  } catch (...) {}
  SET(EnergyCharge)
  SET(MagnetoBomb)
  SET(PlasmaBurst)
  SET(SGBomb)
  SET(GatlingPlasma)
  SET(Monophase)
  SET(Missile)
  SET(ParticleBeam)
  #undef SET
}

/* Saves energy levels for current weapons to ship.info.energy_levels,
 * if the ship is non-NULL and each weapon does not return -1 for its
 * level. If it already exists, ship.info.energy_levels is deleted.
 */
static void saveEnergyLevels(Ship* s) noth {
  if (!s) return;
  if (s->typeName.empty()) return;
  if (!conf[s->typeName.c_str()]["info"].exists("energy_levels"))
    conf[s->typeName.c_str()]["info"].add("energy_levels", Setting::TypeGroup);
  try {
    #define SAVE(name) \
      if (-1 != weapon_getEnergyLevel(s, Weapon_##name)) {\
        if (!conf[s->typeName.c_str()]["info"]["energy_levels"].exists(#name)) \
          conf[s->typeName.c_str()]["info"]["energy_levels"].add(#name, Setting::TypeInt) = \
            weapon_getEnergyLevel(s, Weapon_##name); \
        else \
        conf[s->typeName.c_str()]["info"]["energy_levels"][#name] = \
          weapon_getEnergyLevel(s, Weapon_##name); \
      }

    SAVE(EnergyCharge)
    SAVE(MagnetoBomb)
    SAVE(PlasmaBurst)
    SAVE(SGBomb)
    SAVE(GatlingPlasma)
    SAVE(Monophase)
    SAVE(Missile)
    SAVE(ParticleBeam)
    #undef SAVE
  } catch (...) {
    //Delete offending group and try again
    conf[s->typeName.c_str()]["info"].remove("energy_levels");
    saveEnergyLevels(s);
  }
  conf.modify(s->typeName.c_str());
}

char compositionBuffer[COMPOSITION_BUFFER_SZ] = {0};
bool isCompositionBufferInUse = false;
unsigned compositionBufferIndex = 0;

HumanController::HumanController(Ship* s)
: Controller(s), spunThisFrame(false),
  timeSinceRetarget(0),
  horizMag(0), vertMag(0), warpMouse(false),
  mapBoundaryWarning(audio::root),
  capacitorWarning(audio::root),
  powerWarning(audio::root),
  heatWarning(audio::root),
  heatDanger(audio::root),
  shieldUp(audio::root),
  shieldDown(audio::root),
  currentWeapon(selectFirstWeapon())
{
  DigitalAction nullDigAct = { {0}, false, NULL, NULL };
  for (unsigned int i=0; i < sizeof(mouseButton)/sizeof(DigitalAction); ++i) {
    mouseButton[i]=nullDigAct;
    timeSinceMouseButtonRepeat[i]=0;
  }
  for (unsigned int i=0; i < sizeof(keyboard)/sizeof(DigitalAction); ++i) {
    keyboard[i]=nullDigAct;
    timeSinceKeyboardRepeat[i]=0;
  }

  mouseHoriz.act=mouseVert.act=NULL;
  mouseHoriz.recentre=mouseVert.recentre=true;

  analogueRotationLastDSec=0;

  if (s) setEnergyLevels(s);
}

Weapon HumanController::selectFirstWeapon() const noth {
  if (ship==NULL) return Weapon_EnergyCharge;
  #define TEST(type) if (weapon_getEnergyLevel(ship, type)!=-1) return type
  TEST(Weapon_Missile);
  TEST(Weapon_Monophase);
  TEST(Weapon_GatlingPlasma);
  TEST(Weapon_PlasmaBurst);
  TEST(Weapon_EnergyCharge);
  TEST(Weapon_SGBomb);
  TEST(Weapon_MagnetoBomb);
  #undef TEST
  return Weapon_EnergyCharge;
}

using action::keysPressed;
using action::buttonsPressed;

HumanController::~HumanController() {
  //For any key or button that has any meaning, reset it
  for (unsigned int i=0; i<sizeof(keyboard)/sizeof(DigitalAction); ++i)
    if (keysPressed[i]) {
      //if (keyboard[i].off) keyboard[i].off(ship, keyboard[i].datum);
      keysPressed[i]=false;
    }
  for (unsigned int i=0; i<sizeof(mouseButton)/sizeof(DigitalAction); ++i)
    if (buttonsPressed[i]) {
      //if (mouseButton[i].off) mouseButton[i].off(ship, mouseButton[i].datum);
      buttonsPressed[i]=false;
    }

  if (ship) saveEnergyLevels(ship);
}

void HumanController::motion(SDL_MouseMotionEvent* e) noth {
  float cx=screenW/2.0f, cy=screenH/2.0f;
  //We want left to be negative
  float dx=e->x-cx;
  //We want up to be positive
  float dy=cy-e->y;

  if (dx!=0 && mouseHoriz.act) {
    float mag=dx/cx;
    mag*=mouseHoriz.sensitivity;
    horizMag=mag;
  }

  if (dy!=0 && mouseVert.act) {
    float mag=dy/cy;
    mag*=mouseVert.sensitivity;
    vertMag=mag;
  }

  warpMouseX=mouseHoriz.recentre? (int)cx : e->x;
  warpMouseY=mouseVert.recentre?  (int)cy : e->y;
  warpMouse=true;
}

void HumanController::button(SDL_MouseButtonEvent* e) noth {
  action::spunThisFrame=spunThisFrame;
  DigitalAction& action=mouseButton[e->button];
  if (e->state == SDL_PRESSED) {
    buttonsPressed[e->button]=true;
    if (action.on) {
      action.on(ship, action.datum);
      timeSinceMouseButtonRepeat[e->button]=0;
    }
  } else {
    buttonsPressed[e->button]=false;
    if (action.off) action.off(ship, action.datum);
  }
  spunThisFrame=action::spunThisFrame;
}

void HumanController::key(SDL_KeyboardEvent* e) noth {
  if (!isCompositionBufferInUse) {
    action::spunThisFrame=spunThisFrame;
    DigitalAction& action=keyboard[e->keysym.sym];
    if (e->state == SDL_PRESSED) {
      keysPressed[e->keysym.sym]=true;
      if (action.on) {
        action.on(ship, action.datum);
        timeSinceKeyboardRepeat[e->keysym.sym]=0;
      }
    } else {
      keysPressed[e->keysym.sym]=false;
      if (action.off) action.off(ship, action.datum);
    }
    spunThisFrame=action::spunThisFrame;
  } else {
    //Composing
    if (e->state != SDL_PRESSED) return;
    switch (e->keysym.sym) {
      case SDLK_RETURN:
        //Post message
        global_chat::post(compositionBuffer);
        //fall through
      case SDLK_ESCAPE:
        //Discard message
        isCompositionBufferInUse=false;
        break;
      case SDLK_BACKSPACE:
        if (compositionBufferIndex) {
          memmove(compositionBuffer+compositionBufferIndex-1,
                  compositionBuffer+compositionBufferIndex,
                  strlen(compositionBuffer)-compositionBufferIndex+1);
          --compositionBufferIndex;
        } break;
      case SDLK_DELETE: {
        unsigned len=strlen(compositionBuffer);
        if (compositionBufferIndex < len) {
          memmove(compositionBuffer+compositionBufferIndex,
                  compositionBuffer+compositionBufferIndex+1,
                  len-compositionBufferIndex+1);
        }
      } break;
      case SDLK_LEFT:
        if (compositionBufferIndex) --compositionBufferIndex;
        break;
      case SDLK_RIGHT:
        if (compositionBufferIndex < strlen(compositionBuffer)) ++compositionBufferIndex;
        break;
      default: if (e->keysym.unicode) {
        //Do nothing if at max size
        unsigned len=strlen(compositionBuffer);
        if (len >= COMPOSITION_BUFFER_SZ-1) return;
        //OK, insert
        memmove(compositionBuffer+compositionBufferIndex+1, compositionBuffer+compositionBufferIndex,
                len - compositionBufferIndex+1); //Add one to copy term NUL as well
        compositionBuffer[compositionBufferIndex++] = (char)e->keysym.unicode;
      } break;
    }
  }
}

void HumanController::update(float et) noth {
  /* Subtract the current time from the decisecond rotation */
  if (analogueRotationLastDSec > 0) {
    analogueRotationLastDSec -= STD_ROT_RATE*et;
    if (analogueRotationLastDSec < 0) analogueRotationLastDSec=0;
  }

  action::spunThisFrame=spunThisFrame;
  if (horizMag!=0 && mouseHoriz.act) {
    if (horizMag < -mouseHoriz.limit) horizMag = -mouseHoriz.limit;
    else if (horizMag > +mouseHoriz.limit) horizMag = +mouseHoriz.limit;
    mouseHoriz.act(ship, horizMag, mouseHoriz.recentre);
  }
  if (vertMag!=0 && mouseVert.act) {
    if (vertMag < -mouseVert.limit) vertMag = -mouseVert.limit;
    else if (vertMag > +mouseVert.limit) vertMag = +mouseVert.limit;
    mouseVert.act(ship, vertMag, mouseVert.recentre);
  }

  if (mouseHoriz.recentre) horizMag=0;
  if (mouseVert .recentre) vertMag =0;

  if (warpMouse) SDL_WarpMouse(warpMouseX, warpMouseY);
  warpMouse=false;

  action::lastFrameTime=et;
  spunThisFrame=false;

  unsigned oldTSR = timeSinceRetarget;
  timeSinceRetarget+=et;
  unsigned newTSR = timeSinceRetarget;
  if (newTSR > 1024 && oldTSR <= 1024)
    targetBlacklist.clear();
  //Se if we just wrapped around a microfortnight
  if (oldTSR%1024 > newTSR%1024 && ship && ship->target.ref) {
    //Can we still see the target?
    radar_t::iterator begin, end;
    ship->radarBounds(begin,end);
    if (end == find(begin, end, ship->target.ref))
      retarget();
  }

  //Automatically retarget if current target is dead
  if (ship && (!ship->target.ref || !((Ship*)ship->target.ref)->hasPower())) retarget();

  for (unsigned int i=0; i<sizeof(mouseButton)/sizeof(DigitalAction); ++i) {
    if (buttonsPressed[i] && mouseButton[i].on && mouseButton[i].repeat &&
        ((timeSinceMouseButtonRepeat[i]+=et)>=REPEAT_T || mouseButton[i].fastRepeat)) {
      mouseButton[i].on(ship, mouseButton[i].datum);
      timeSinceMouseButtonRepeat[i]=0;
    }
  }

  for (unsigned int i=0; i<sizeof(keyboard)/sizeof(DigitalAction); ++i) {
    if (keysPressed[i] && keyboard[i].on && keyboard[i].repeat &&
        ((timeSinceKeyboardRepeat[i]+=et)>=REPEAT_T || keyboard[i].fastRepeat)) {
      keyboard[i].on(ship, keyboard[i].datum);
      timeSinceKeyboardRepeat[i]=0;
    }
  }

  //Handle general sounds
  if (ship) {
    capacitorWarning.setOn(ship->getCapacitancePercent() < 0.15f && ship->getMaximumCapacitance() > 30000);
    powerWarning.setOn(ship->getPowerUsagePercent() > 1.0f);
    float xx = ship->getX() + 10000*ship->getVX(), xy = ship->getY() + 10000*ship->getVY();
    mapBoundaryWarning.setOn(xx < 0 || xy < 0 || xx > ship->getField()->width || xy > ship->getField()->height);
    float lowHeatThresh = (MAX_TEMP - 2*(MAX_TEMP-WARN_TEMP))/MAX_TEMP, highHeatThresh = WARN_TEMP/MAX_TEMP;
    heatWarning.setOn(ship->getHeatPercent() > lowHeatThresh && ship->getHeatPercent() < highHeatThresh);
    heatDanger.setOn(ship->getHeatPercent() >= highHeatThresh);
    //Find the weakest shield
    float weakestShield = 2.0f;
    const std::vector<Shield*>& shields = ship->getShields();
    for (unsigned i=0; i<shields.size(); ++i)
      if (shields[i]->getStrength()/shields[i]->getMaxStrength() < weakestShield)
        weakestShield = shields[i]->getStrength()/shields[i]->getMaxStrength();
    shieldDown.setOn(weakestShield < 0.01f);
    //Don't play ShieldUp when we load if we have no shields (in which case weakestShield
    //stays at 2)
    shieldUp.setOn(weakestShield > 0.99f && !shields.empty());
  }
}

void HumanController::retarget() noth {
  timeSinceRetarget=0;

  Ship* bestTarget=NULL;
  float bestScore=(float)1.0e32;
  radar_t::iterator begin, end;
  for (ship->radarBounds(begin,end); begin != end; ++begin) {
    Ship* go = *begin;
    if (go==ship) continue;
    if (!go->hasPower()) continue;
    if (getAlliance(ship->insignia, go->insignia) != Enemies) continue;

    bool blacklisted=false;
    for (unsigned int j=0; j<targetBlacklist.size() && !blacklisted; ++j)
      if (go == targetBlacklist[j]) blacklisted=true;
    if (blacklisted) continue;

    //Score is relative to distance and angle
    float dx=go->getX()-ship->getX(),
          dy=go->getY()-ship->getY();
    float dist=sqrt(dx*dx + dy*dy),
          angle=atan2(dy, dx);
    float da=angle-ship->getRotation();
    while (da > pi) da-=2*pi;
    while (da <-pi) da+=2*pi;

    float score=dist*(fabs(da)+0.15f);

    //Penalty of 1000 points if outside screen
    if (go->getX() < cameraX1 || go->getX() > cameraX2
    ||  go->getY() < cameraY1 || go->getY() > cameraY2)
      score += 1000;

    if (score<bestScore) {
      bestScore=score;
      bestTarget=go;
    }
  }

  //If NULL, we may have blacklisted everything, so clear and try again
  if (!bestTarget) {
    if (targetBlacklist.size()) {
      targetBlacklist.clear();
      retarget(); return;
    } else {
      //Nothing. There is nothing we can do.
      //However, we do need to set the target to NULL in case we retargeted
      //due to a stealth ship escaping
      ship->target.assign(NULL);
      return;
    }
  }

  //Blacklist so we don't continuously retarget it
  targetBlacklist.push_back(bestTarget);
  //Set the target
  audio::root.add(new audio::ChangeTarget);
  ship->target.assign(bestTarget);
}

void HumanController::hc_conf_bind() {
  ActionDatum thisPair;
  thisPair.pair.first=this;
  thisPair.pair.second.ptr=NULL;
  ActionDatum thisPtr;
  thisPtr.ptr=this;
  DigitalAction da_accel = {{0}, false, false, action::accel_on, action::accel_off},
                da_decel = {{0}, false, false, action::decel_on, action::decel_off},
                da_turn  = {{0}, true,  true,  action::turn_on, NULL},
                da_fire  = {thisPtr, true, true, action::fire_on, NULL},
                da_throt = {{0}, true, false, action::throttle_on, NULL},
                da_weapn = {thisPair, false, false, action::set_current_weapon_on, NULL},
                da_power = {thisPair, true, false, action::adjust_weapon_power_on, NULL},
                da_retgt = {thisPtr, false, false, action::retarget_on, NULL},
                da_selfd = {{0}, false, false, action::selfDestruct_on, NULL},
                da_stlth = {{0}, false, false, action::stealth_on, NULL},
                da_comps = {thisPtr, false, false, action::compose_message_on, NULL};
  bind( da_accel, "accel" );
  bind( da_decel, "decel" );
  bind( da_turn , "rotate", Float, false, getFloatLimit(STD_ROT_RATE));
  bind( da_fire , "fire", NoParam );
  bind( da_throt, "throttle", Float, false, getFloatLimit(1) );
  bind( da_weapn, "set current weapon", CString, true);
  bind( da_power, "adjust weapon power", Integer, true, getIntLimit(0xFFFF) );
  bind( da_retgt, "retarget" );
  bind( da_selfd, "self destruct" );
  bind( da_stlth, "stealth" );
  bind( da_comps, "compose" );
};
