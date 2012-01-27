/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the BackgroundObject class.
 *
 * This is mainly only useful for the StarField class.
 */
#ifndef BACKGROUND_OBJECT_HXX_
#define BACKGROUND_OBJECT_HXX_

#include <vector>
#include <string>

#include <GL/gl.h>

#include "src/opto_flags.hxx"

/** A BackgroundObject is a graphic shown with the stars in the background.
 * Each instance is effectively a singleton of its particular type. When
 * the StarField takes one in, it calls the randomize() function to place
 * the object on axes, but otherwise leaves it unchanged.
 *
 * Before this can be used, the loadBackgroundObjects() function must be
 * called.
 */
class BackgroundObject {
  private:
  GLuint texture;
  const std::string filename;

  //Each of these is inverse (ie, 1/dist), from 0..1
  //At the natural distance, the graphic takes up
  //the screen height specified in natSize
  const float minDist, natDist, maxDist, natSize;

  float x, y, z;

  public:
  /**
   * Type of object this graphic represents.
   * This is used as a key name when reading images/bg/bg.rc.
   */
  const char*const className;

  /**
   * Constructs a new BackgroundObject with the specified parms.
   * @param filenm Filename from which to read the image
   * @param min Minimum Z value
   * @param nat "Natural" Z value
   * @param max Maximum Z value
   * @param clazz Value for the className member.
   */
  BackgroundObject(const char* filenm, float min, float nat, float max, float sz, const char* clazz) :
    texture(0), filename(filenm),
    minDist(min), natDist(nat), maxDist(max), natSize(sz), className(clazz)
  {}
  ~BackgroundObject();

  /** Load the object.
   * @return NULL on success, error description on failure.
   */
  const char* load() noth;

  /** Frees the texture used by this object. */
  void unload() noth;

  /** Randomize the location of the object, with the given
   * X and Y bounds.
   * @parm maxx The maximum X value
   * @parm maxy The maximum Y value
   */
  void randomize(float maxx, float maxy) noth;

  /** Returns the X coordinate. */
  float getX() const noth;
  /** Returns the Y coordinate. */
  float getY() const noth;

  /** Returns the inverse Z coordinate (parallax) of the object. */
  float getZ() const noth;

  /** Paints the object.
   * It is assumed that GL has already been
   * translated in compensation for the camera. The X and Y
   * coördinates of the reference must be passed for parallax
   * calculations.
   * @param x The X coordinate of the camera
   * @param y The Y coordinate of the camera
   */
  void draw(float x, float y) const noth;
};

/** A count of the number of graphic classes within images/bg/bg.rc */
extern unsigned backgroundObjectClassCount;

/** A vector of all extant BackgroundObjects. */
extern std::vector<BackgroundObject*> backgroundObjects;

/** Loads the objects, as defined in images/bg/bg.rc .
 * @return true if successful, false on failure.
 */
bool loadBackgroundObjects();

#endif /*BACKGROUND_OBJECT_HXX_*/
