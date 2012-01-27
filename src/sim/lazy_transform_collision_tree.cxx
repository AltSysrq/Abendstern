/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/lazy_transform_collision_tree.hxx
 */

/*
 * lazy_transform_collision_tree.cxx
 *
 *  Created on: 15.06.2011
 *      Author: jason
 */

#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <iostream>
#include <cstdio>

#if !defined(DEBUG) && !defined(NDEBUG)
#define NDEBUG
#endif
#include <cassert>

#include "lazy_transform_collision_tree.hxx"
#include "collision.hxx"

using namespace std;

#define MIN_DIVISION_CNT 8
#define MAX_DEPTH 8

LazyTransformCollisionTree::LazyTransformCollisionTree()
: active(NULL)
{ }

LazyTransformCollisionTree::~LazyTransformCollisionTree() {
  free();
}

void LazyTransformCollisionTree::reset(float xo, float yo, float ct, float st) noth {
  if (!active) return;

  xoff = xo;
  yoff = yo;
  cost = ct;
  sint = st;
  transform(active[0], internalSource[0]);
}

void LazyTransformCollisionTree::setData(const CollisionRectangle*const* rects, unsigned cnt) noth {
  if (active) free();

  if (!cnt) return; //No information to work with

  externalSource = rects;

  //First, find the bounding box
  float minx = INFINITY, maxx = -INFINITY, miny = INFINITY, maxy = -INFINITY;
  for (unsigned i=0; i<cnt; ++i) {
    for (unsigned v=0; v<4; ++v) {
      minx = min(minx, rects[i]->vertices[v].first);
      maxx = max(maxx, rects[i]->vertices[v].first);
      miny = min(miny, rects[i]->vertices[v].second);
      maxy = max(maxy, rects[i]->vertices[v].second);
    }
  }

  //Construct tree
  struct TreeBox {
    float minx, maxx, miny, maxy;
    vector<const CollisionRectangle*> contained;
    TreeBox* sub0, * sub1;
    LazyTransformCollisionTree::Level* level;

    TreeBox(unsigned depth, float x0, float x1, float y0, float y1,
            const CollisionRectangle*const* rects, unsigned cnt)
    : minx(x0), maxx(x1), miny(y0), maxy(y1), sub0(NULL), sub1(NULL)
    {
      //Find the items that intersect with this box.
      //A simple vertex-in-box test will work well enough; while there will
      //be cases that this doesn't catch, they will rarely cause a significant
      //problem
      for (unsigned i=0; i<cnt; ++i) for (unsigned v=0; v<4; ++v) {
        if (rects[i]->vertices[v].first >= x0 && rects[i]->vertices[v].first < x1
        &&  rects[i]->vertices[v].second >= y0 && rects[i]->vertices[v].second < y1) {
          contained.push_back(rects[i]);
          v=4; //Done with this one
        }
      }

      //Do we subdivide?
      //Do so if we still contain more than MIN_DIVISION_CNT and are at a depth
      //less than MAX_DEPTH
      if (contained.size() > MIN_DIVISION_CNT && depth < MAX_DEPTH) {
        //Divide along bigger dimension or X if equal
        float xs = x1-x0, ys = y1-y0;
        sub0 = new TreeBox(depth+1, x0, (xs >= ys? (x1+x0)/2 : x1), y0, (ys > xs? (y1+y0)/2 : y1),
                           &contained[0], contained.size());
        sub1 = new TreeBox(depth+1, (xs >= ys? (x1+x0)/2 : x0), x1, (ys > xs? (y1+y0)/2 : y0), y1,
                           &contained[0], contained.size());
      }
    }

    ~TreeBox() {
      if (sub0) delete sub0;
      if (sub1) delete sub1;
    }

    /* Call deleteEmpty() on children, then delete any subs that are empty.
     * If sub0 is removed but sub1 is not, move sub1 to sub0.
     */
    void deleteEmpty() {
      if (sub0) sub0->deleteEmpty();
      if (sub1) sub1->deleteEmpty();
      if (sub0 && sub0->contained.empty()) {
        delete sub0;
        sub0 = NULL;
      }
      if (sub1 && sub1->contained.empty()) {
        delete sub1;
        sub1 = NULL;
      }

      if (sub1 && !sub0) sub0 = sub1, sub1 = NULL;
    }

    /* Call prune() on children; then, if either child has only one child,
     * replace it with its child.
     * Assumes that single-children children have sub0 and not sub1
     * (guaranteed by deleteEmpty()).
     */
    void prune() {
      if (sub0) sub0->prune();
      if (sub1) sub1->prune();
      if (sub0 && sub0->sub0 && !sub0->sub1) {
        TreeBox* s = sub0;
        sub0 = sub0->sub0;
        s->sub0 = NULL;
        delete s;
      }
      if (sub1 && sub1->sub0 && !sub1->sub1) {
        TreeBox* s = sub1;
        sub1 = sub1->sub0;
        s->sub0 = NULL;
        delete s;
      }
    }

    /* Counts the nodes in the tree. */
    unsigned count() const {
      return 1 + (sub0? sub0->count() : 0) + (sub1? sub1->count() : 0);
    }

    /* Counts the CollisionRectangles that will be required for the
     * entire tree.
     */
    unsigned rectsNeeded() const {
      if (sub0)
        //Both children will exist in this case
        return 1 + sub0->rectsNeeded() + sub1->rectsNeeded();
      else
        return 1 + contained.size();
    }

    /* Assignes a Level object to this TreeBox. */
    LazyTransformCollisionTree::Level* assignLevel(LazyTransformCollisionTree::Level* level) {
      this->level = level;
      if (sub0)
        return sub1->assignLevel(sub0->assignLevel(level+1));
      else
        return level+1;
    }

    /* Initialises the source rectangle for this box. */
    void setupRectangle(CollisionRectangle& rect) const {
      rect.vertices[0].first  = minx;
      rect.vertices[0].second = miny;
      rect.vertices[1].first  = maxx;
      rect.vertices[1].second = miny;
      rect.vertices[2].first  = maxx;
      rect.vertices[2].second = maxy;
      rect.vertices[3].first  = minx;
      rect.vertices[3].second = maxy;
      rect.recurse = &LazyTransformCollisionTree::thunkTransform;
      rect.data = level;
      rect.radius = sqrt((maxx-minx)*(maxx-minx) + (maxy-miny)*(maxy-miny));
    }

    /* Sets this branch of the tree up, including source rectangles
     * and level objects.
     * The rectangle for the top of the tree must be setup manually.
     */
    void setup(LazyTransformCollisionTree* caller,
               CollisionRectangle*& internalSource,
               const CollisionRectangle**& mapping,
               unsigned& ix) {
      if (sub0) {
        //We have children
        level->that = caller;
        level->offset = ix;
        level->len = 2;
        *mapping++ = internalSource;
        sub0->setupRectangle(*internalSource++);
        *mapping++ = internalSource;
        sub1->setupRectangle(*internalSource++);
        ix += 2;

        sub0->setup(caller, internalSource, mapping, ix);
        sub1->setup(caller, internalSource, mapping, ix);
      } else {
        //No children, just source rectangles
        level->that = caller;
        level->offset = ix;
        level->len = contained.size();
        for (unsigned i = 0; i<contained.size(); ++i)
          *mapping++ = contained[i];
        ix += contained.size();
      }
    }
  } tree(0, minx, maxx, miny, maxy, rects, cnt);
  tree.deleteEmpty();
  tree.prune();
  if (tree.sub0 && !tree.sub1) {
    //Handle special case
    //Everything is within one-half of the
    //top-level
    delete tree.sub0;
    tree.sub0 = NULL;
  }

  assert(tree.sub0 || !tree.sub1);
  assert(!tree.contained.empty());

  unsigned internalCount = tree.count();
  unsigned totalCount = tree.rectsNeeded();

  //OK, allocate memory
  //_destroy variables will be passed to setup and will not be valid
  //after that call
  internalSource = new CollisionRectangle[internalCount];
  const CollisionRectangle** sourceMapping_destroy = new const CollisionRectangle*[totalCount];
  sourceMapping = sourceMapping_destroy;
  active = new CollisionRectangle[totalCount];
  activeIndirect = new CollisionRectangle*[totalCount];
  levels = new Level[internalCount];

  //Allocate levels
  tree.assignLevel(levels);
  //Manually setup top rectangle
  top.push_back(active);
  tree.setupRectangle(*internalSource);
  //Run through the rest of the tree
  unsigned index_destroy = 1;
  ++sourceMapping_destroy; //Move to index 1 (index 0 is never used)
  CollisionRectangle* internalSource_destroy = internalSource+1;
  tree.setup(this, internalSource_destroy, sourceMapping_destroy, index_destroy);

  //Setup activeDirect
  for (unsigned i=0; i<totalCount; ++i)
    activeIndirect[i] = active+i;
}

void LazyTransformCollisionTree::free() noth {
  if (!active) return;

  delete[] internalSource;
  delete[] sourceMapping;
  delete[] active;
  delete[] activeIndirect;
  delete[] levels;
  //externalSource is owned by our owner
  top.clear();

  active = NULL;
}

void LazyTransformCollisionTree::transform(CollisionRectangle& dst, const CollisionRectangle& src) noth {
  for (unsigned i=0; i<4; ++i) {
    dst.vertices[i].first  = xoff + cost*src.vertices[i].first - sint*src.vertices[i].second;
    dst.vertices[i].second = yoff + sint*src.vertices[i].first + cost*src.vertices[i].second;
  }

  dst.radius = src.radius;
  dst.recurse = src.recurse;
  dst.data = src.data;
}

void LazyTransformCollisionTree::thunkTransform(CollisionRectangle* rect,
                                                CollisionRectangle*const*& nxt, unsigned& cnt) {
  Level* lvl = (Level*)rect->data;
  nxt = lvl->that->activeIndirect + lvl->offset;
  cnt = lvl->len;

  for (unsigned i=0; i<cnt; ++i)
    lvl->that->transform(lvl->that->active[i+lvl->offset], *lvl->that->sourceMapping[i+lvl->offset]);

  rect->recurse = &LazyTransformCollisionTree::thunkCached;
}

void LazyTransformCollisionTree::thunkCached(CollisionRectangle* rect,
                                             CollisionRectangle*const*& nxt, unsigned& cnt) {
  Level* lvl = (Level*)rect->data;
  nxt = lvl->that->activeIndirect + lvl->offset;
  cnt = lvl->len;
}
