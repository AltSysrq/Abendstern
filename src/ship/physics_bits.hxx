/**
 * @file
 * @author Jason Lingle
 * @brief Defines bits used to manage ship physics.
 *
 * See the source of this file for explanations on the _BIT and _BITS constants.
 */

/*
 * physics_bits.hxx
 *
 *  Created on: 16.05.2011
 *      Author: jason
 */

#ifndef PHYSICS_BITS_HXX_
#define PHYSICS_BITS_HXX_

/** The type used to contain ship physics bits. */
typedef unsigned physics_bits;

/* The ship-global and cell-specific bits are stored in the high
 * and low halves of the physics_bits.
 */
/** Mask for ship-specific physics bits */
#define PHYS_SHIP_MASK ((physics_bits)0xFFFF0000)
/** Mask for cell-specific physics bits */
#define PHYS_CELL_MASK ((physics_bits)0x0000FFFF)
/** Mask for all physics bits */
#define PHYS_EVERYTHING (PHYS_SHIP_MASK | PHYS_CELL_MASK)

/* The individual bit values for each property.
 * These should only be used for testing properties;
 * for clearing, see the bits values, which include
 * dependencies.
 */
#define PHYS_CELL_MASS_BIT                      ((physics_bits)(1<< 0))
#define PHYS_CELL_LOCATION_PROPERTIES_BIT       ((physics_bits)(1<< 1))
#define PHYS_CELL_THRUST_BIT                    ((physics_bits)(1<< 2))
#define PHYS_CELL_TORQUE_BIT                    ((physics_bits)(1<< 3))
#define PHYS_CELL_COOLING_BIT                   ((physics_bits)(1<< 4))
#define PHYS_CELL_POWER_BIT                     ((physics_bits)(1<< 5))
#define PHYS_CELL_POWER_PROD_BIT                ((physics_bits)(1<< 6))
#define PHYS_CELL_HEAT_BIT                      ((physics_bits)(1<< 7))
#define PHYS_CELL_CAPAC_BIT                     ((physics_bits)(1<< 8))
#define PHYS_CELL_DS_EXIST_BIT                  ((physics_bits)(1<< 9))
#define PHYS_CELL_DS_NEAREST_BIT                ((physics_bits)(1<<10))
#define PHYS_CELL_ROT_THRUST_BIT                ((physics_bits)(1<<11))
#define PHYS_CELL_REINFORCEMENT_BIT             ((physics_bits)(1<<12))
#define PHYS_SHIP_COORDS_BIT                    ((physics_bits)(1<<16))
#define PHYS_SHIP_MASS_BIT                      ((physics_bits)(1<<17))
#define PHYS_SHIP_INERTIA_BIT                   ((physics_bits)(1<<18))
#define PHYS_SHIP_THRUST_BIT                    ((physics_bits)(1<<19))
#define PHYS_SHIP_TORQUE_BIT                    ((physics_bits)(1<<20))
#define PHYS_SHIP_ROT_THRUST_BIT                ((physics_bits)(1<<21))
//Includes heating
#define PHYS_SHIP_COOLING_BIT                   ((physics_bits)(1<<22))
#define PHYS_SHIP_POWER_BIT                     ((physics_bits)(1<<23))
#define PHYS_SHIP_CAPAC_BIT                     ((physics_bits)(1<<24))
#define PHYS_SHIP_ENGINE_INVENTORY_BIT          ((physics_bits)(1<<25))
#define PHYS_SHIP_WEAPON_INVENTORY_BIT          ((physics_bits)(1<<26))
#define PHYS_SHIP_SHIELD_INVENTORY_BIT          ((physics_bits)(1<<27))
#define PHYS_SHIP_DS_INVENTORY_BIT              ((physics_bits)(1<<28))
#define PHYS_SHIP_PBL_INVENTORY_BIT             ((physics_bits)(1<<29))

#define PHYS_CELL_ALL ((physics_bits)0x00001FFF)
#define PHYS_SHIP_ALL ((physics_bits)0x2FFF0000)
#define PHYS_ALL (PHYS_CELL_ALL | PHYS_SHIP_ALL)

/* The BITS constants are used for clearing physics bits, and
 * encompass all dependencies on the change.
 * Dispersion shields cannot simply be cleared, since the linked-list
 * must be manually maintained, so the BITS constants for these
 * must NOT be used for clearing.
 */
#define PHYS_SHIP_MASS_BITS (PHYS_SHIP_MASS_BIT | PHYS_SHIP_COORDS_BITS)
#define PHYS_SHIP_INERTIA_BITS PHYS_SHIP_INERTIA_BIT
#define PHYS_SHIP_COOLING_BITS PHYS_SHIP_COOLING_BIT
#define PHYS_SHIP_TORQUE_BITS PHYS_SHIP_TORQUE_BIT
#define PHYS_SHIP_THRUST_BITS (PHYS_SHIP_THRUST_BIT | PHYS_SHIP_TORQUE_BITS)
#define PHYS_SHIP_ROT_THRUST_BITS PHYS_SHIP_ROT_THRUST_BIT
#define PHYS_SHIP_POWER_BITS PHYS_SHIP_POWER_BIT
#define PHYS_SHIP_CAPAC_BITS PHYS_SHIP_CAPAC_BIT
#define PHYS_CELL_DS_NEAREST_BITS PHYS_CELL_DS_NEAREST_BIT
#define PHYS_CELL_DS_EXIST_BITS (PHYS_CELL_DS_EXIST_BIT | PHYS_SHIP_DS_INVENTORY_BITS)
#define PHYS_CELL_ROT_THRUST_BITS (PHYS_CELL_ROT_THRUST_BIT | PHYS_SHIP_ROT_THRUST_BITS)
#define PHYS_CELL_TORQUE_BITS (PHYS_CELL_TORQUE_BIT | PHYS_SHIP_TORQUE_BITS | PHYS_CELL_ROT_THRUST_BITS)
#define PHYS_CELL_THRUST_BITS (PHYS_CELL_THRUST_BIT | PHYS_SHIP_THRUST_BITS | PHYS_CELL_TORQUE_BITS)
#define PHYS_CELL_COOLING_BITS (PHYS_CELL_COOLING_BIT | PHYS_SHIP_COOLING_BITS)
#define PHYS_CELL_HEAT_BITS (PHYS_CELL_HEAT_BIT | PHYS_SHIP_COOLING_BITS)
#define PHYS_CELL_LOCATION_PROPERTIES_BITS (PHYS_CELL_LOCATION_PROPERTIES_BIT | \
                                            PHYS_CELL_TORQUE_BITS | PHYS_CELL_ROT_THRUST_BITS | \
                                            PHYS_SHIP_INERTIA_BITS)
#define PHYS_CELL_MASS_BITS (PHYS_CELL_MASS_BIT | PHYS_SHIP_MASS_BITS | PHYS_CELL_LOCATION_PROPERTIES_BITS | \
                             PHYS_SHIP_INERTIA_BITS)
#define PHYS_CELL_POWER_BITS (PHYS_CELL_POWER_BIT | PHYS_CELL_POWER_PROD_BIT | PHYS_SHIP_POWER_BITS)
#define PHYS_CELL_POWER_PROD_BITS PHYS_CELL_POWER_BITS
#define PHYS_CELL_CAPAC_BITS (PHYS_CELL_CAPAC_BIT | PHYS_SHIP_CAPAC_BITS)
#define PHYS_CELL_REINFORCEMENT_BITS PHYS_CELL_REINFORCEMENT_BIT
#define PHYS_SHIP_DS_INVENTORY_BITS (PHYS_SHIP_DS_INVENTORY_BIT | PHYS_CELL_DS_NEAREST_BITS)
#define PHYS_SHIP_COORDS_BITS (PHYS_SHIP_COORDS_BIT | PHYS_CELL_LOCATION_PROPERTIES_BITS)
#define PHYS_SHIP_ENGINE_INVENTORY_BITS (PHYS_SHIP_ENGINE_INVENTORY_BIT | PHYS_SHIP_TORQUE_BITS \
                                        | PHYS_SHIP_THRUST_BITS | PHYS_SHIP_ROT_THRUST_BITS)
#define PHYS_SHIP_WEAPON_INVENTORY_BITS PHYS_SHIP_WEAPON_INVENTORY_BIT
#define PHYS_SHIP_SHIELD_INVENTORY_BITS PHYS_SHIP_SHIELD_INVENTORY_BIT
#define PHYS_SHIP_PBL_INVENTORY_BITS PHYS_SHIP_PBL_INVENTORY_BIT

#endif /* PHYSICS_BITS_HXX_ */
