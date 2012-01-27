/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/sys/a/cloaking_device.hxx
 */

/*
 * cloaking_device.cxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#include "cloaking_device.hxx"
#include "src/ship/sys/system_textures.hxx"

CloakingDevice::CloakingDevice(Ship* s)
: ShipSystem(s, system_texture::cloakingDevice, Classification_Cloak, Standard, Large)
{
  _supportsStealthMode=true;
}

unsigned CloakingDevice::mass() const noth {
  return 150;
}
