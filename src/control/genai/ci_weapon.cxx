/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/ci_weapon.hxx
 */

/*
 * ci_weapon.cxx
 *
 *  Created on: 01.11.2011
 *      Author: jason
 */

#include <cmath>

#include "src/ship/sys/launcher.hxx"
#include "src/ship/cell/cell.hxx"
#include "ci_weapon.hxx"
#include "src/globals.hxx"

using namespace std;

void cortex_input::weaponGetInputs(const Launcher* l, float* dst) {
  dst[WeaponInputOffsets::wx] = l->container->getX();
  dst[WeaponInputOffsets::wy] = l->container->getY();
  dst[WeaponInputOffsets::wt] = l->getLaunchAngle();
  //Normalise theta
  float theta = dst[WeaponInputOffsets::wt];
  theta = fmod(theta, 2*pi);
  if (theta > pi) theta -= 2*pi;
  else if (theta <= -pi) theta += 2*pi;
  dst[WeaponInputOffsets::wt] = theta;
}
