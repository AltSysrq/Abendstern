/**
 * @file
 * @author Jason Lingle
 * @brief Contains functions and definitions for tracking the
 * alliances and insignias of ships.
 */

#ifndef INSIGNIA_HXX_
#define INSIGNIA_HXX_

#include <map>

class Ship;

/**
 * Indicates the alliance/animosity between two Ships.
 */
enum Alliance {
  Neutral=0,
  Allies,
  Enemies
};

/** Returns a value to use as an insignia. If the name
 * has not yet been bound, it is created.
 * "neutral" always returns 0.
 */
unsigned long insignia(const char*);

/** Clears the list of insignias and alliances. */
void clear_insignias();

/** Returns the alliance of two insignias */
Alliance getAlliance(unsigned long, unsigned long);

/** Overrides the default alliance between the
 * two insignias.
 */
void setAlliance(Alliance, unsigned long, unsigned long);

#endif /*INSIGNIA_HXX_*/
