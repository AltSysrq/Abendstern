/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/game_field.hxx
 */

#include <GL/gl.h>
#include <typeinfo>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

#include "game_field.hxx"
#include "game_object.hxx"
#include "src/background/explosion.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"
#include "src/camera/effects_handler.hxx"
#include "src/graphics/cmn_shaders.hxx"
#include "src/graphics/matops.hxx"
#include "src/graphics/glhelp.hxx"
#include "src/graphics/asgi.hxx"
#include "src/net/network_assembly.hxx"
#include "src/exit_conditions.hxx"
using namespace std;

GameField::GameField(float w, float h)
: fieldClock(0), fieldClockMicro(0), width(w), height(h),
  effects(&nullEffectsHandler), perfectRadar(false),
  networkAssembly(NULL)
{
  nw=(int)ceil(w);
  nh=(int)ceil(h);

  //Boundaries
  const float l(0.1f);
  shader::quickV vertices[24] = {
    {{{0,0}}}, {{{l,h-l}}}, {{{0,h}}},
    {{{l,l}}}, {{{l,h-l}}}, {{{0,0}}},
    {{{0,0}}}, {{{w,0}}}, {{{w-l,l}}},
    {{{0,0}}}, {{{w-l,l}}}, {{{l,l}}},
    {{{w,0}}}, {{{w,h}}}, {{{w-l,h-l}}},
    {{{w,0}}}, {{{w-l,h-l}}}, {{{w-l,l}}},
    {{{0,h}}}, {{{l,h-l}}}, {{{w-l,h-l}}},
    {{{0,h}}}, {{{w-l,h-l}}}, {{{w,h}}},
  };
  if (!headless) {
    vao=newVAO();
    glBindVertexArray(vao);
    vbo=newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::quick->setupVBO();
  }
}
GameField::~GameField() {
  for (unsigned int i=0; i<objects.size(); ++i) delete objects[i];
  for (unsigned int i=0; i<toInject.size(); ++i) delete toInject[i];
  for (unsigned int i=0; i<toInsert.size(); ++i) delete toInsert[i];
  for (vector<GameObject*>::iterator it=toDelete.begin();
       it != toDelete.end();
       ++it)
    delete *it;
  for (vector<GameObject*>::iterator it=deleteNextFrame.begin();
       it != deleteNextFrame.end();
       ++it)
    delete *it;

  if (!headless) {
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
  }
}

void GameField::update(float time) {
  fieldClockMicro += time;
  fieldClock += (Uint32)fieldClockMicro;
  fieldClockMicro -= (Uint32)fieldClockMicro;

  if (size()>10000 && false) {
    cout << "FATAL: Detected object count explosion.\nDump of object types:" << endl;
    for (unsigned int i=0; i<size(); ++i)
      cout << i << ": " << typeid(*objects[i]).name() << endl;
    exit(EXIT_THE_SKY_IS_FALLING);
  }
  unsigned int MAX_FRAME_LENGTH=conf["conf"]["vframe_granularity"];
  currentFrameTimeLeft=currentFrameTime=time;
  float subTime=time;
  while (subTime>MAX_FRAME_LENGTH) {
    currentVFrameLast = (subTime - MAX_FRAME_LENGTH <= 0);
    currentVFrameLast = (subTime == 0);
    updateImpl<false>(MAX_FRAME_LENGTH);
    subTime-=MAX_FRAME_LENGTH;
    currentFrameTimeLeft=subTime;
  }
  if (subTime>0) {
    currentVFrameLast=true;
    updateImpl<false>(subTime);
  }

  updateImpl<true>(time);
  expool.update(time);

  //Enable decorations
  for (unsigned int i=0; i<objects.size(); ++i) objects[i]->okToDecorate();

  //Delete objects to be deleted this frame
  for (vector<GameObject*>::iterator it=toDelete.begin();
       it != toDelete.end();
       ++it)
    delete *it;
  //Enqueue objects to delete next frame
  toDelete = deleteNextFrame;

  deleteNextFrame.clear();
}

/* Performs a collision check on the two objects. If
 * either is destroyed, its pointer is nullified after
 * being deleted.
 * The caller must check for this.
 */
#ifdef PROFILE
#define inline
#endif
inline bool check_collision(deque<GameObject*>&, GameObject*, GameObject*, unsigned&, unsigned) noth HOT FAST;
/* Performs a collision detection between two objects. The deque of objects, the two
 * objects themselves, and references to their initial indices are provided.
 * If either object dies, it is removed from the deque and del()ed appropriately.
 * The indices are updated so that they are still valid.
 * Returns true if object A is still alive, false otherwise.
 */
inline bool check_collision(deque<GameObject*>& objects, GameObject* ap, GameObject* bp,
                            unsigned& aix, unsigned bix) noth {
#ifdef inline
#undef inline
#endif
  //No colision if both are remote
  if (ap->isRemote && bp->isRemote) return true;

  CollisionResult first=ap->checkCollision(bp);
  switch(first) {
    case NoCollision: return true;
    case YesCollision: goto collision;
    case UnlikelyCollision: switch(bp->checkCollision(ap)) {
      case NoCollision:
      case MaybeCollision:
      case UnlikelyCollision: return true;
      case YesCollision: goto collision;
    }
    case MaybeCollision: switch(bp->checkCollision(ap)) {
      case NoCollision:
      case UnlikelyCollision: return true;
      case MaybeCollision:
      case YesCollision: goto collision;
    }
  }

  collision:
  bool keepA=ap->collideWith(bp),
       keepB=bp->collideWith(ap);
  if (!keepA) {
    objects.erase(objects.begin() + aix);
    --aix;
    assert(!ap->isRemote);
    ap->del();
  }
  if (!keepB) {
    objects.erase(objects.begin() + bix);
    assert(!bp->isRemote);
    bp->del();
    --aix;
  }
  return keepA;
}

template <bool decor>
void GameField::updateImpl(float time) noth {
  isUpdatingDecorative=decor;
  /* There are several #ifdef..#endif sections regarding PROFILE.
   * We only want to split updateImpl(float) up if PROFILE is defined.
   */
  #ifdef PROFILE
  updateImpl_update<decor>(time);
  updateImpl_collision<decor>(time);
  updateImpl_begin<decor>(time);
}

template <bool decor>
void GameField::updateImpl_update(float time) noth {
  #endif //ifdef PROFILE
  //Update all objects first, /then/ do collision detection
  for (unsigned int it=0; it<objects.size(); ++it) {
    GameObject* obj=objects[it];
    if (obj->isDecorative() != decor) continue;
    if (!obj->update(time)) {
      assert(!obj->isRemote);
      obj->del();
      objects.erase(objects.begin() + it--);
      continue;
    }
    //Kill if out of bounds
    if (obj->getX()<0 || obj->getX()>=width ||
        obj->getY()<0 || obj->getY()>=height) {
      obj->collideWith(obj);
      assert(!obj->isRemote);
      obj->del();
      objects.erase(objects.begin() + it--);
      continue;
    }

    //Compute collision information
    float rad=obj->getRadius();
    obj->ci.left=obj->x-rad;
    obj->ci.right=obj->x+rad;
    obj->ci.upper=obj->y+rad;
    obj->ci.lower=obj->y-rad;
  }

  //Resort list with gnome sort
  if (objects.size() > 1) {
    deque<GameObject*>::iterator end=objects.end()-1;
    for (deque<GameObject*>::iterator i=objects.begin(), j=i+1; i != end;) {
      if ((*i)->ci.right > (*j)->ci.right) {
        //Swap
        GameObject* tmp = *i;
        *i=*j;
        *j=tmp;
        if (i != objects.begin()) --i, --j;
      } else ++i, ++j;
    }
  }
  #ifdef PROFILE
}

template<bool decor>
void GameField::updateImpl_collision(float time) noth {
  #endif //ifdef profile
  isInCollisionSubcycle=true;

  for (unsigned aix=0; aix < objects.size(); ++aix) {
    GameObject* a = objects[aix];
    //Don't need to check isDead, since dead objects
    //are now removed immediately
    //if (a->ci.isDead) continue;
    if (!a->includeInCollisionDetection) continue;
    if (!decor && a->isDecorative()) continue;
    for (unsigned bix = aix-1; bix < aix && objects[bix]->ci.right >= a->ci.left; --bix) {
      GameObject* b = objects[bix];
      if (!decor && b->isDecorative()) continue;
      if (decor && !(a->isDecorative() || b->isDecorative())) continue;
      //if (b->ci.isDead) continue;
      if (!b->includeInCollisionDetection) continue;
      if ((a->ci.upper >= b->ci.lower && a->ci.upper <= b->ci.upper)
      ||  (a->ci.lower >= b->ci.lower && a->ci.lower <= b->ci.upper)
      ||  (b->ci.lower >= a->ci.lower && b->ci.lower <= b->ci.upper)) {
        if (!check_collision(objects, a, b, aix, bix)) goto next_a;
      }
    }
    next_a:;
  }

  isInCollisionSubcycle=false;

  for (unsigned i=0; i<toInject.size(); ++i)
    doInject(toInject[i], time);
  toInject.clear();
  #ifdef PROFILE
}

template<bool decor>
void GameField::updateImpl_begin(float time) noth {
  #endif //ifdef PROFILE
  //Add any to-be-inserted objects
  for (unsigned i=0; i<toInsert.size(); ++i) {
    GameObject* go=toInsert[i];
    doAdd(go);
  }
  toInsert.clear();
}

void GameField::draw() {
  expool.draw();
  for (int layer=0; layer<3; ++layer) {
    for (unsigned int i=0; i<objects.size(); ++i) {
      GameObject* obj=objects[i];
      //Correct draw layer
      if (obj->drawLayer != layer) continue;
      //Make sure visible
      if (obj->getX()-obj->getRadius() > cameraX2 ||
          obj->getX()+obj->getRadius() < cameraX1 ||
          obj->getY()-obj->getRadius() > cameraY2 ||
          obj->getY()+obj->getRadius() < cameraY1) continue;
      obj->draw();
      //Test collision bounds
#if 0
      const vector<CollisionRectangle*>& v(*obj->getCollisionBounds());
      struct CollisionTest {
        static void doit(CollisionRectangle*const* v, unsigned cnt) {
          asgi::colour(1,0,1,0.05f);
          for (unsigned r=0; r<cnt; ++r) {
            asgi::begin(asgi::Quads);
            for (unsigned s=0; s<4; ++s)
              asgi::vertex(v[r]->vertices[s].first, v[r]->vertices[s].second);
            asgi::end();

            if (v[r]->recurse) {
              CollisionRectangle*const* sr;
              unsigned sc;
              v[r]->recurse(v[r], sr, sc);
              doit(sr,sc);
            }
          }
        }
      };
      CollisionTest::doit(&v[0], v.size());
#endif /* disabled */
    }
  }

  shader::quickU uni;
  uni.colour=Vec4(1,1,0,0.5f);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  shader::quick->activate(&uni);
  glDrawArrays(GL_TRIANGLES, 0, 24);
}

void GameField::add(GameObject* go) noth {
  toInsert.push_back(go);
}

void GameField::addBegin(GameObject* go)  noth {
  toInsert.push_back(go);
}

void GameField::add(Explosion* go) noth {
  expool.add(go);
}

void GameField::addBegin(Explosion* go) noth {
  expool.add(go);
}

void GameField::remove(GameObject* go) noth {
  deque<GameObject*>::iterator it = find(objects.begin(), objects.end(), go);
  if (it == objects.end()) {
    #ifdef DEBUG
    cerr << "Warning: Ignoring attempt to remove non-added object " << go
         << " from GameField " << this << endl;
    #endif
    return;
  }
  objects.erase(it);
}

void GameField::removeFromInsertQueue(GameObject* go) noth {
  deque<GameObject*>::iterator it = find(toInsert.begin(), toInsert.end(), go);
  if (it != toInsert.end())
    toInsert.erase(it);
}

void GameField::clear() noth {
  for (iterator it=begin(); it != end(); ++it) {
    (*it)->collideWith(*it);
    assert(!(*it)->isRemote);
    (*it)->del();
  }
  objects.clear();
  for (unsigned int i=0; i<toInject.size(); ++i) delete toInject[i];
  toInject.clear();
  for (unsigned int i=0; i<toInsert.size(); ++i) delete toInsert[i];
  toInsert.clear();
}

void GameField::inject(GameObject* go) noth {
  if (!isInCollisionSubcycle
  ||  (go->isDecorative() && !isUpdatingDecorative))
    add(go); //Nothing special to do
  else toInject.push_back(go);
}

unsigned GameField::doAdd(GameObject* go) noth {
  if (networkAssembly)
    networkAssembly->objectAdded(go);

  float rad=go->getRadius();
  go->ci.left=go->x-rad;
  go->ci.right=go->x+rad;
  go->ci.upper=go->y+rad;
  go->ci.lower=go->y-rad;
  //Binary search to find the right location
  unsigned low=0, high=objects.size();
  unsigned mid=0;
  while (high > low) {
    mid = (high+low)/2;
    if (go->ci.right > objects[mid]->ci.right)
      low=mid+1;
    else
      high=mid;
  }
  mid=low;
  objects.insert(objects.begin()+mid, go);
  return mid;
}

void GameField::doInject(GameObject* go, float subTime) noth {
  //unsigned ix = doAdd(go);
  doAdd(go);

  /*
  if (!go->update(subTime)) {
    go->del();
    objects.erase(objects.begin()+ix);
    return; //Nothing else we can do
  }

  //Check previous
  for (unsigned i=ix-1; i<ix && objects[i]->ci.right >= go->ci.left; --i) {
    GameObject* oth=objects[i];
    if (!oth->includeInCollisionDetection) continue;
    if (oth->ci.upper >= go->ci.lower && oth->ci.lower <= go->ci.upper)
      if (!check_collision(objects, go, oth, ix, i)) goto deleted;
  }
  //Check next
  for (unsigned i = ix+1; i < objects.size() && objects[i]->ci.left <= go->ci.right; ++i) {
    GameObject* oth=objects[i];
    if (!oth->includeInCollisionDetection) continue;
    if (oth->ci.upper >= go->ci.lower && oth->ci.lower <= go->ci.upper)
      if (!check_collision(objects, oth, go, i, ix)) goto deleted;
  }
  deleted: return;
  */
}
