/**
 * @file
 * @author Jason Lingle
 * @brief Contains an interface to Abendstern's planet generator.
 */

/*
 * planet_generator.hxx
 *
 *  Created on: 11.03.2011
 *      Author: jason
 */

#ifndef PLANET_GENERATOR_HXX_
#define PLANET_GENERATOR_HXX_

#include <libconfig.h++>

#include "src/core/aobject.hxx"

class Planet;
class GameObject;
class GameField;

/** The Planet Generator is a set of functions that generate
 * new planetary background images, based on a set of parameters.
 * Internally, they use a separate pRNG from the C library to
 * guarantee reproducability across platforms.
 */
namespace planetgen {
  /** The width of the generated images.
   * This is not modifiable.
   */
  static const signed short width = 5376;
  /** The height of the generated images.
   * This is not modifiable.
   */
  static const signed short height = 2048;

  /** Defines the parameters used to generate
   * a planet.
   * All distances are measured in planet-heights,
   * except altitudes, which are meausered relative
   * to maximum altitude (ie, 0=sea level, 1=maximum).
   */
  class Parameters: public AObject {
    public:
    /** Defines the pRNG seed used to generate
     * the planet. Only the lower 31 bits are used.
     */
    unsigned seed;

    /** Defines the number of continent seeds to plant.
     * Continents are areas of land that grow aggressively.
     */
    unsigned continents;
    /** Defines the number of large island seeds to plant.
     * Large islands are areas of land that grow at a medium pace.
     */
    unsigned largeIslands;
    /** Defines the number of small island seeds to plant.
     * Small islands are areas of land that grow slowly.
     * They have a tendancy to be placed near each other, as per
     * the islandGrouping parameter.
     */
    unsigned smallIslands;
    /** Defines the exponential modifier for accepting a
     * new small island seed at any given location.
     * The probability equation is
     *   <code>e^(-islandGrouping*dist)</code>
     * where dist is the distance from a given possible island
     * seed to the nearest already-placed island seed.
     */
    float islandGrouping;

    /** Defines the upward slope of land. To find the elevation
     * of a given land point, the nearest water is located, and
     * this value is multiplied by the distance.
     */
    float landSlope;

    /** Defines the number of ocean seeds to plant.
     * Oceans are areas of water that grow aggressively.
     */
    unsigned oceans;
    /** Defines the number of sea seeds to plant.
     * Seas are areas of water that grow at a medium pace.
     */
    unsigned seas;
    /** Defines the number of lake seeds to plant.
     * Lakes are areas of water that grow slowly.
     */
    unsigned lakes;

    /** Defines the number of rivers to create.
     * Rivers start at a random land point and proceed
     * downhill, choosing random adjacent land pixels
     * if multiple are of the same elevation.
     * The actual number of rivers will vary based on
     * the percent of the planet's surface which is land.
     */
    unsigned rivers;

    /** Defines the number of mountain ranges to create.
     * Mountain ranges are linear stretches of land with
     * mountains spawned along the line, with probability
     * inversely proportional to the distance from the line.
     */
    unsigned mountainRanges;
    /** The amount to multiply landSlope by when creating
     * mountain ranges.
     */
    float mountainSteepness;

    /** Defines the number of "enormous" mountains to create.
     * An enormous mountain has a peak of the maximum possible
     * elevation and equal slopes on all sides of landSlope*2.
     */
    unsigned enormousMountains;

    /** Defines the number of craters to create.
     * Creaters are deformations in altitude, imitating their
     * real-life counterparts.
     */
    unsigned craters;
    /** Defines the maximum radius of any crater. */
    float maxCraterSize;

    /** Defines the temperature, in kelvins, at the solar equator
     * of the planet.
     */
    float equatorTemperature;
    /** Defines the solar equator. 0.5 is the centre. */
    float solarEquator;
    /** Defines the temperature, in kelvins, at the poles. */
    float polarTemperature;
    /** Defines the change in temperature, in kelvins, of
     * proceeding from sea level to the maximum altitude.
     */
    float altitudeTemperatureDelta;
    /** Defines the natural temperature offset of water
     * pixels, in kelvins.
     */
    float waterTemperatureDelta;
    /** Defines the freezing point of water, in kelvins. */
    float freezingPoint;

    /** Defines the base humidity of the planet.
     * This is added to all humidity calculations.
     */
    float humidity;
    /** Defines the multiplier to add to humidity conditions
     * when considering distance from water. Ie,
     *   <code>humidity += vapourTransport/distance</code>
     */
    float vapourTransport;
    /** Defines the multiplier to apply to humidity calculations
     * when considering mountains blocking a source of water.
     * Ie,
     * \verbatim
     *   if (sampleAltitude > locationAltitude)
     *     humidity -= (mountainBlockage*(sampleAltitude-locationAltitude))/dist
     * \endverbatim
     */
    float mountainBlockage;

    /** Defines the humidity required for vegitation to
     * grow. Vegitation only grows at locations above
     * the freezing point.
     */
    float vegitationHumidity;

    /** Defines the number of cities to create.
     * All cities will be placed on land.
     */
    unsigned cities;
    /** Defines the maximum size of a city, in
     * screen area (ie, square planet-heights).
     */
    float maxCitySize;
    /** Defines the tendancy of cities to cluster
     * together. The probability of accepting
     * a city in a given location is
     * <code>  e^(-cityGrouping*distance)</code>
     * where distance is the distance to the nearest already-
     * established city.
     */
    float cityGrouping;

    /** Defines the colour of water, XRGB (ie, lower 24 bits),
     * in machine byte order.
     */
    unsigned waterColour;
    /** Defines the colour of vegitation, XRGB (ie, lower 24 bits),
     * in machine byte order.
     */
    unsigned vegitationColour;
    /** Defines the colour of the planet at sea level, XRGB (ie, lower 24
     * bits) in machine byte order.
     */
    unsigned lowerPlanetColour;
    /** Defines the colour of the planet at maximum altitude, XRGB
     * (ie, lower 24 bits) in machine byte order.
     */
    unsigned upperPlanetColour;
  };

  /** Allocates resources necessary for planet generation
   * and begins generating on a separate thread.
   * Returns immediately.
   * The Parameters is copied, so the caller may free it
   * immediately after this call.
   * It is not possible to render more than one planet at
   * once. One of kill, save, or planetify MUST be called
   * after a call to begin and before the next call to
   * begin.
   */
  void begin(const Parameters&);
  static inline void begin(const Parameters* p) { begin(*p); }

  /** Returns a string description of the current stage
   * of planet generation.
   */
  const char* what();
  /** Returns the progress, 0..1, of planet generation. */
  float progress();
  /** Returns true if generation is complete or if no
   * generation is occurring.
   */
  bool done();

  /** Immediately terminates the generator thread and
   * deallocates all resources used by generation.
   */
  void kill();

  /** Finishes planet generation by saving images to the specified
   * pair of filenames.
   * On failure, an exception is thrown.
   * After success of this function call, all resources associated
   * with the planet generator are destroyed.
   */
  void save(const char* day, const char* night);

  /** Finishes planet generation by returning a Planet with
   * the generated images and the specified arguments.
   * After this function call, all resources associated with
   * the planet generator are destroyed.
   */
  Planet* planetify(GameObject*, GameField*, float revtime, float orbittime, float height, float twilight);
}

#endif /* PLANET_GENERATOR_HXX_ */
