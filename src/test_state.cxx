/**
 * @file
 * @author Jason Lingle
 *
 * @brief Implements test_state.hxx.
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <map>
#include <cstdio>
#include <ctime>
#include <libconfig.h++>

#include <GL/gl.h>
#include <SDL.h>
#include "test_state.hxx"
#include "globals.hxx"
#include "background/planet.hxx"
#include "background/star_field.hxx"
#include "background/nebula.hxx"
#include "secondary/aggregate_set.hxx"
#include "ship/insignia.hxx"
#include "camera/hud.hxx"
#include "camera/dynamic_camera.hxx"
#include "control/hc_conf.hxx"
#include "control/human_controller.hxx"
#include "control/ai/aictrl.hxx"
#include "ship/ship.hxx"
#include "ship/shipio.hxx"
#include "exit_conditions.hxx"
#include "graphics/asgi.hxx"
#include "audio/ship_mixer.hxx"
#include "audio/synth.hxx"
#include "secondary/namegen.hxx"

using namespace std;
using namespace libconfig;
using namespace hc_conf;

static bool bulletTime=false, snailTime=false, frameByFrame=false;
static float timeUntilNextFrame=250;
static TestState* that;
static bool run=true;

#define FIELD_SIZE test_state::size

namespace test_state {
  const char * gameClass;
  int humanShip;
  Mode mode;
  int size;
  string getShipFile() {
    Setting& ships=conf["hangar"][(const char*)gameClass];
    string str((const char*)ships[rand() % ships.getLength()]);
    return str;
  }

  void* add_to_int(void* target, void* amount) {
    *((int*)target)+=*((int*)amount);
    return amount;
  }

  void playerDeath(Ship* s, bool del) {
    that->craft_ptr->shipExistenceFailure=NULL;
    that->newCinema();
    frameByFrame=false;
  }

  void enemyDeath(Ship* s, bool del) {
    delete s->effects;
    s->shipExistenceFailure=NULL;
    if (mode != LastManStanding && mode != ManVsWorld) that->newEnemy();
  }

  void action_bulletTime(Ship* s, ActionDatum& d) {
    bulletTime=!bulletTime;
  }
  void action_snailTime(Ship* s, ActionDatum& d) {
    snailTime=!snailTime;
  }
  void action_frameByFrame(Ship* s, ActionDatum& d) {
    debug_dynamicCameraDisableVibration = frameByFrame=!frameByFrame;
  }
  void action_pause(Ship* s, ActionDatum& d) {
    debug_freeze=!debug_freeze;
  }
  void action_exit(Ship* s, ActionDatum& d) {
    run=false;
  }
  void action_zoom(Ship* s, ActionDatum& d) {
    DynamicCamera* cam=(DynamicCamera*)that->env.cam;
    cam->setZoom(cam->getZoom()+d.amt);
  }

  //Shared radars for team modes (map insignia to radar)
  map<unsigned long, radar_t*> sharedRadars;
  radar_t* getSharedRadar(unsigned long i) {
    if (!sharedRadars[i]) sharedRadars[i] = new radar_t;
    return sharedRadars[i];
  }
};
using namespace test_state;

class CinematicView : public GameObject {
  private:
  float timeLeft, rotation;

  public:
  CinematicView(GameField* f, GameObject& o) : GameObject(f, o.getX(), o.getY(), o.getVX(), o.getVY()),
                                               timeLeft(3000), rotation(o.getRotation()) {}
  virtual bool update(float time) noth {
    x+=vx*time;
    y+=vy*time;

    timeLeft-=time;
    if (timeLeft>0) return true;
    else {
      debug_dynamicCameraDisableVibration=false;
      if (mode != LastManStanding) that->newCraft();
      else run=false;
      return false;
    }
  }
  virtual void draw() noth {}
  virtual float getRadius() const noth { return 0.01f; }
  virtual float getRotation() const noth { return rotation; }
  virtual bool collideWith(GameObject* other) noth {
    if (other==this) {
      debug_dynamicCameraDisableVibration=false;
      if (mode != LastManStanding) that->newCraft();
      else run=false;
      return false;
    } else return true;
  }
};

TestState::TestState(test_state::Background bck)
: env(new DynamicCamera(NULL, &env.field), FIELD_SIZE, FIELD_SIZE),
  numTestAIDeaths(1), numStdAIDeaths(1)
{
  that=this;
  if (bck == test_state::Nebula) {
    ::Nebula* n = new ::Nebula(NULL, &env.field, 0.80f, 0.60f, 0.25f, 0.05f, 100);
    env.stars = n;
    n->setForceMultiplier(0.05f);
    //n->setPressureEquation("abs cos x", true);
    //n->setFlowEquation("_ / (* y 1) sqrt(+ * x x * y y)", "/ (* x 1) sqrt(+ * x x * y y)", true);
    //n->setFlowEquation("0", "cos(* 100 x)", true);
    //n->setVelocityResetTime(100);
  } else if (bck == test_state::StarField) {
    env.stars = new ::StarField(NULL, &env.field, 1000);
  } else {
    env.stars = new ::Planet(NULL, &env.field, "images/earthday_post.png", "images/earthnight_post.png",
                            -15*60*1000, -3*60*1000, 2.5f, 0.1f);
  }

  run=true;
  bulletTime=snailTime=frameByFrame=false;
  DigitalAction da_exit = {{0}, false, false, action_exit, NULL},
                da_fram = {{0}, false, false, action_frameByFrame, NULL},
                da_fast = {{0}, false, false, action_snailTime, NULL},
                da_slow = {{0}, false, false, action_bulletTime, NULL},
                da_halt = {{0}, false, false, action_pause, NULL};
  bind( da_exit, "__ exit" );
  bind( da_fram, "__ frameXframe" );
  bind( da_fast, "__ fast" );
  bind( da_slow, "__ slow" );
  bind( da_halt, "__ halt" );
  clear_insignias();

  audio::ShipMixer::init();

  ((DynamicCamera*)env.cam)->hc_conf_bind();
  craft_ptr=NULL;
  human=NULL;
  newCraft();

  int numEnemies=conf["conf"]["testing_numenemies"];
  for (int i=0; i<numEnemies; ++i) newEnemy();

  if (conf["conf"][(const char*)conf["conf"]["control_scheme"]]["analogue"]["horiz"]["recentre"]
  &&  conf["conf"][(const char*)conf["conf"]["control_scheme"]]["analogue"]["vert" ]["recentre"])
    SDL_ShowCursor(SDL_DISABLE);
  else
    SDL_ShowCursor(SDL_ENABLE);
  SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
}

TestState::~TestState() {
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  audio::ShipMixer::end();
}

void TestState::newEnemy() {
  Ship* enemy_ptr;
  string enemyMount(getShipFile());
  cout << "Load enemy: " << enemyMount << endl;
  Setting& enemySetting(conf[enemyMount]["info"]);
  try {
    enemy_ptr=loadShip(&env.field, enemyMount.c_str());
  } catch (runtime_error& e) {
    cerr << e.what() << endl;
    exit(EXIT_MALFORMED_DATA);
  }

  Ship& enemy=*enemy_ptr;
  float red=rand()/(float)RAND_MAX;
  float green=rand()/(float)RAND_MAX * (1-red);
  float blue=1-red-green;
  enemy.setColour(red, green, blue);
  enemy.shipExistenceFailure=enemyDeath;
  switch (mode) {
    case StandardTeam:      enemy.insignia=insignia(enemySetting["alliance"][rand() %
                                                    enemySetting["alliance"].getLength()]); break;
    case HomogeneousTeam:   enemy.insignia=insignia(enemyMount.c_str()); break;
    case HeterogeneousTeam: enemy.insignia=rand()%5+1; break;
    case ManVsWorld:        enemy.insignia=2; break;
    case LastManStanding:   //fall through
    case FreeForAll:        enemy.insignia=reinterpret_cast<unsigned long>(&enemy); break;
  }
  enemy.controller = new AIControl(enemy_ptr, conf["ai"]["test"]);
  if (mode != FreeForAll && mode != LastManStanding)
    enemy.setRadar(getSharedRadar(enemy.insignia));

  enemy.tag = namegenGet();
  enemy.tag += " (";
  enemy.tag += (const char*)conf[enemyMount]["info"]["name"];
  enemy.tag += ")";

  env.field.addBegin(enemy_ptr);
}

void TestState::newCraft() {
  string humanMount=(humanShip>=0?
                     string((const char*)conf["hangar"][(const char*)gameClass][humanShip])
                   : getShipFile());
  Setting& humanSetting=conf[humanMount]["info"];
  const char* filename=humanMount.c_str();
  cout << "Load human: " << filename << endl;
  try {
    craft_ptr=loadShip(&env.field, humanMount.c_str());
  } catch (runtime_error& e) {
    cerr << e.what() << endl;
    exit(EXIT_MALFORMED_DATA);
  }

  craft.shipExistenceFailure=playerDeath;
  Setting& colour(conf["conf"]["ship_colour"]);
  craft.setColour(colour[0],colour[1],colour[2]);
  switch (mode) {
    case StandardTeam:      craft.insignia=insignia(humanSetting["alliance"][0]); break;
    case HomogeneousTeam:   craft.insignia=insignia(filename); break;
    case HeterogeneousTeam: craft.insignia=rand()%5+1; break;
    case ManVsWorld:        craft.insignia=1; break;
    case FreeForAll: break;
    case LastManStanding: break;
  }
  if (human) delete human;
  human=new HumanController(&craft);
  craft.controller=human;
  human->hc_conf_bind();
  if (mode != FreeForAll && mode != LastManStanding)
    craft.setRadar(getSharedRadar(craft.insignia));

  try {
    hc_conf::configure(human, (const char*)conf["conf"]["control_scheme"]);
  } catch (ConfigException& e) {
    cout << e.what() << endl;
    exit(EXIT_MALFORMED_CONFIG);
  }

  delete craft.effects;
  craft.effects=(DynamicCamera*)env.cam;

  env.setReference(&craft, true);
  env.field.addBegin(&craft);
  //Forget 0,0
  if (env.stars) {
    env.stars->repopulate();
  }

  craft.enableSoundEffects();
}

void TestState::newCinema() {
  CinematicView* cine=new CinematicView(&env.field, craft);
  env.setReference(cine, false);
  env.field.addBegin(cine);
  human=new HumanController(NULL);
  human->hc_conf_bind();
  try {
    hc_conf::configure(human, (const char*)conf["conf"]["control_scheme"]);
  } catch (ConfigException& e) {
    cout << e.what() << endl;
    exit(EXIT_MALFORMED_CONFIG);
  }
}

GameState* TestState::update(float et) {
  sprintf(hud_user_messages::msg0, "FPS: %d", frameRate);
  sprintf(hud_user_messages::msg1, "OBJ: %d", env.field.size());
  unsigned shipCount=0;
  for (unsigned i=0; i<env.field.size(); ++i)
    if (env.field[i]->getClassification() == GameObject::ClassShip
    &&  ((Ship*)env.field[i])->hasPower())
      ++shipCount;
  sprintf(hud_user_messages::msg2, "SHIP: %d", shipCount);
  time_t rawtime;
  time(&rawtime);
  strftime(hud_user_messages::msg3, 16, "%H:%M:%S", localtime(&rawtime));

  static Ship nullShip(&env.field);
  if (!run) return this;
  if (debug_freeze) return NULL;
  if (bulletTime) et/=10;
  if (snailTime) et*=3;
  if (frameByFrame) {
    timeUntilNextFrame-=et;
    if (timeUntilNextFrame>0) return NULL;
    timeUntilNextFrame=250;
    et=5;
  }

  env.update(et);

  return NULL;
}

void TestState::draw() {
  if (run) {
    env.draw();
  }
}

void TestState::motion(SDL_MouseMotionEvent* e) {
  if (human) human->motion(e);
}

void TestState::mouseButton(SDL_MouseButtonEvent* e) {
  if (human) human->button(e);
}

void TestState::keyboard(SDL_KeyboardEvent* e) {
  if (human) human->key(e);
}
