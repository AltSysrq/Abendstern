/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ShipIO interface
 *
 * ShipIO is the next-generation (replacing ShipParse) method of storing Ships
 * to files. Unlike ShipParse, which uses a custom, non-extensible format, ShipIO
 * utilizes libconfig++ to allow new features to be added without breaking old
 * files.
 *
 * ShipIO represents the ship as a table of cells with interconnections, instead of
 * a tree with implicit connections. In order to allow the upcomming NG-ShipEditor
 * to easily add and remove cells, cells are given names instead of indices.
 *
 * From the top-level of the file, the format is:
 * \verbatim
 * info                 Group
 * info.name            String, name of ship
 * info.version         Version number, in case of incompatible changes (current=0)
 * info.alliance        String array, alliances the ship can be a part of.
 * info.class           String, class of ship (eg, "C", "B", ..)
 * info.bridge          Name of bridge cell.
 * info.reinforcement   Float, reinforcement level
 * info.mass            Mass of ship. This is automatically set upon load.
 * info.colour          Optional float array(3); preferred colour for the ship.
 * info.author          Optional string, name of creator
 * info.time            Optional group
 * info.time.creation   Optional string, format YYYY.MM.DD HH:mm:ss
 * info.time.modification Optional string, format YYYY.MM.DD HH:mm:ss
 * info.revision        Optional integer, number of times ship has been saved
 * info.old_format      Optional string, contents of ShipParse .ship file if converted
 * cells                Group
 * cells.<name>         Group for cell <name>
 * cells.*.type         String, type of cell (square, circle, right, equil).
 * cells.*.neighbours   String array, listing names of neighbours in order.
 * cells.*.s0           Optional group for system 0
 * cells.*.s1           Optional group for system 1
 * cells.*.s*.type      String, typename of the system.
 * cells.*.s*.orient    Integer, orientation of the system.
 * cells.*.s*.capacity  Integer; for capacitors, capacity.
 * cells.*.s*.radius    Float; for shields, radius
 * cells.*.s*.strength  Float; for shields, strength
 * cells.*.s*.turbo     Boolean; indicates the turbo status of GatlingPlasmaBurstLaunchers
 * cells.*.damage       Optional float; predamage given cell.
 * celll.*.not_bridge   Optional bool; if true, indicates that the cell does not
 *                      participate as a bridge, despite being marked as such (ie, the ship
 *                      is always a fragment).
 * \endverbatim
 */


/*
 * shipio.hxx
 *
 *  Created on: 18.07.2010
 *      Author: Jason Lingle
 */

#ifndef SHIPIO_HXX_
#define SHIPIO_HXX_

class Ship;
class GameField;

/** The current version of ShipIO.
 *
 * 0 = Prerelease
 *
 * Any version can read anything before it.
 */
#define SHIPIO_VERSION 0

/** Loads a ship from the given mounted libconfig++ file.
 * @param field The field in which the Ship will live
 * @param root The ConfReg root to use
 * @return The ship loaded
 * @throw NoSuchSettingException if ConfRegi indicates a problem
 * @throw libconfig::SettingException on invalid configuration
 * @throw std::runtime_error if logical problems are encountered
 */
Ship* loadShip(GameField* field, const char* root);

/** Saves a ship into the given mounted libconfig++ file, first deleting
 * everything in that mount. This does not sync the mount.
 * The information written is minimal. Name and alliance are nondescriptive
 * name determined from the mountpoint. Class is determined automatically by
 * the highest-class system present.
 * Due to the cell naming system used, no more than 26^4 = 456,976 cells
 * can be present in the Ship.
 */
void saveShip(Ship*, const char*);
#endif /* SHIPIO_HXX_ */
