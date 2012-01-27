/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the StarField Background class and associated functions.
 */

#ifndef STAR_FIELD_HXX_
#define STAR_FIELD_HXX_

#include <GL/gl.h>

#include "src/core/aobject.hxx"
#include "background_object.hxx"
#include "src/camera/effects_handler.hxx"
#include "src/opto_flags.hxx"
#include "background.hxx"

class Star;
class GameField;
class GameObject;

#define STARFIELD_SIZE 3
//The maximum zoom out (1/5) should only be 3/4, but
//1 should still be 1
//To keep things simple, we'll stick with a linear function
#define SF_FAKE_ZOOM (5.0f*cameraZoom/16.0f + 11.0f/16.0f)


/** The StarField class simply manages a collection of Stars use as a Background.
 * It also creates the graphics information used for drawing them.
 */
class StarField: public Background {
  private:
  float oldX, oldY;
  int starCount;
  GameObject* reference;
  GameField* field;
  Star* stars;

  BackgroundObject** bkgObjects;
  int numBkgObjects;

  //The backdrops of pixel-stars
  GLuint backdropVAO[4], backdropVBO[4];
  unsigned backdropSz[4];

  //The current glare colour from explosions
  float glareR, glareG, glareB;

  public: virtual ~StarField();

  public:
  /** Constructs a new StarField with the given parms.
   *
   * @param ref Initial reference GameObject; may be NULL
   * @param field The GameField this is a background to
   * @param count The number of Stars to use
   */
  StarField(GameObject* ref, GameField* field, int count);

  virtual void update(float time) noth;
  virtual void draw() noth;
  /* This MUST be called to maintain the reference.
   * It also must also be called before deleting
   * the reference. However, you can set this to NULL
   * to indicate no updating.
   */
  virtual void updateReference(GameObject* reference, bool reset) noth;
  /* Called to dispose all stars and create new ones. */
  virtual void repopulate() noth;

  virtual void explode(Explosion*) noth;
};

/** Loads the textures for the stars.
 * This MUST be called before any stars can be drawn.
 */
void initStarLists();

#endif /*STAR_FIELD_HXX_*/
