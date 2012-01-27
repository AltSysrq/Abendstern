/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_attack/aim_basic_aim.hxx
 */

#include <libconfig.h++>
#include <cmath>
#include <algorithm>

#include "aim_basic_aim.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/globals.hxx"
#include "src/ship/ship.hxx"
#include "src/ship/sys/ship_system.hxx"

using namespace std;

AIM_BasicAim::AIM_BasicAim(AIControl* c, const libconfig::Setting& s)
: AIModule(c),
  speed(s.exists("attack_speed")? s["attack_speed"] : 0.0005f)
{ }

void AIM_BasicAim::action() {
  Ship* t = (Ship*)ship->target.ref;
  if (!t) return;
  if (!weapon_exists(ship, (Weapon)controller.getCurrentWeapon())) return;

  WeaponHUDInfo whudi;
  whudi.dist = 0; //Don't care about this, but it can't be uninitialised
  //Get speed
  weapon_getWeaponInfo(ship, (Weapon)controller.getCurrentWeapon(), whudi);
  /* We use the velocity
   * difference between the our ship and the target to
   * predect the time it would take the projectile to reach
   * the target (assuming perfect aim), then place the reticule
   * there. We have the following variables:
   *   ps   Vector, position of our ship
   *   pt   Vector, position of the target
   *   vs   Vector, our ship velocity
   *   vt   Vector, target ship velocity
   *   sp   Scalar, speed of projectile
   *   va   Unit vector, our ship's angle
   *
   * Relative velocity is (vs-vt). We are only concerned with the
   * velocity to or from us, so that is
   *   dot(norm(ps-pt),(vs-vt))
   * Relative speed between projectile and ship is then
   *   sp + dot(norm(ps-pt),(vs-vt))
   * And time is
   *  t = len(ps-pt) / (sp - dot(norm(ps-pt),(vs-vt)))
   * We can then determine "future would-be collision location" as
   *   pf = ps + t*(sp*va+vs);
   * The extrapolated position of the target is
   *   px = pt + vt*t
   * And our future position
   *   py = ps + vs*t
   * The effective position of the target is then
   *   pz = px - (ps-py)
   */
  Ship* s=ship;
  float psx = s->getX(), psy = s->getY();
  float ptx = t->getX(), pty = t->getY();
  float vsx = s->getVX(), vsy = s->getVY();
  float vtx = t->getVX(), vty = t->getVY();
  //float vax = cos(s->getRotation());
  //float vay = sin(s->getRotation());
  float sp = whudi.speed;
  float dx = ptx-psx;
  float dy = pty-psy;
  float d = sqrt(dx*dx + dy*dy);
  float denom = (sp + (dx/d*(vsx-vtx) + dy/d*(vsy-vty)));
  float tt = (fabs(denom) > 1.0e-30f? d / denom : 0);
  //float pfx = psx + tt*(sp*vax+vsx);
  //float pfy = psy + tt*(sp*vay+vsy);
  float pxx = ptx + vtx*tt;
  float pxy = pty + vty*tt;
  float pyx = psx + vsx*tt;
  float pyy = psy + vsy*tt;
  float pzx = pxx - pyx + psx;
  float pzy = pxy - pyy + psy;

  //(pzx,pzy) is our new target point
  controller.sstat("suggest_target_offset_x", pzx-t->getX());
  controller.sstat("suggest_target_offset_y", pzy-t->getY());

  float dzx = pzx - s->getX();
  float dzy = pzy - s->getY();
  float zang = atan2(dzy, dzx);// - controller.gstat("suggest_angle_offset", 0.0f);
  controller.setTargetTheta(zang);
  float shotQuality = cos(zang-ship->getRotation()
                          - controller.gstat("suggest_angle_offset", 0.0f));
  if (shotQuality > 0) shotQuality *= shotQuality;
  controller.sstat("shot_quality", shotQuality);

  if (vsx*vsx + vsy*vsy < speed*speed) {
    if (shotQuality > 0.7f)
      ship->configureEngines(true, false, shotQuality*controller.gglob("max_throttle", 1.0f));
    else if (shotQuality > 0.0f)
      ship->configureEngines(true, true,
                             shotQuality*0.5f*controller.gglob("max_throttle",1.0f));
    else
      ship->configureEngines(false, true, controller.gglob("max_brake", 1.0f));
  } else {
    ship->configureEngines(false, true, controller.gglob("max_brake", 1.0f));
  }
}

static AIModuleRegistrar<AIM_BasicAim> registrar("attack/basic_aim");
