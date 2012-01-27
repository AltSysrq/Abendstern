/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/aimod.hxx
 */

/*
 * aimod.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include "aimod.hxx"
#include "aictrl.hxx"

AIModule::AIModule(AIControl* i)
: controller(*i), ship(i->ship)
{ }
