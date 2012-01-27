/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Antenna network abstraction class.
 */

/*
 * antenna.hxx
 *
 *  Created on: 09.12.2011
 *      Author: jason
 */

#ifndef ANTENNA_HXX_
#define ANTENNA_HXX_

#include <asio.hpp>

#include "globalid.hxx"
#include "src/core/aobject.hxx"

class Tuner;

/**
 * The Antenna class abstracts away some of the immediate interface
 * for sending network packets. It also handles concerns like port
 * and IP version selection, as well as determining the local peer's
 * global-unique id.
 *
 * While primarily used as a singleton, there are no conflicts in
 * using multiple instances of this class.
 */
class Antenna: public AObject {
};


#endif /* ANTENNA_HXX_ */
