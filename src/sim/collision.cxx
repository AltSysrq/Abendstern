/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/collision.hxx
 */

#include <cmath>
#include <utility>
#include <iostream>
#include <memory>
#include <vector>

#include "collision.hxx"
#include "src/globals.hxx"
#include "game_object.hxx"
#include "src/opto_flags.hxx"
using namespace std;

enum DotSide {
  Negative, Collinear, Positive
};

static inline pair<float,float> operator-(const pair<float,float>& a, const pair<float,float>& b) {
  pair<float,float> r;
  r.first=a.first-b.first;
  r.second=a.second-b.second;
  return r;
}

//Rather arbitrary, but less than one pixel
#define DOT_COLLINEAR_THRESHHOLD 0
static inline DotSide dot(const pair<float,float>& rot, const pair<float,float>& test, const pair<float,float>& v) {
  float product = (rot.first*(test.first-v.first) + rot.second*(test.second-v.second));
  if      (product < -DOT_COLLINEAR_THRESHHOLD) return Negative;
  else if (product > +DOT_COLLINEAR_THRESHHOLD) return Positive;
  else return Collinear;
}

static inline float dot(const pair<float,float>& a, const pair<float,float>& b) {
  return a.first*b.first + a.second*b.second;
}

static bool rectanglesCollideImpl(const CollisionRectangle&, const CollisionRectangle&) noth;

bool rectanglesCollide(const CollisionRectangle& a, const CollisionRectangle& b) noth {
  return rectanglesCollideImpl(a,b) && rectanglesCollideImpl(b,a);
}

bool rectanglesCollideImpl(const CollisionRectangle& a, const CollisionRectangle& b) noth {
  //We'll use the separating axis test for this
  //If there is any edge for which all of the other polygon's vertices are on the
  //same side, either that edge is a separator OR the other is contained inside
  //this one
  //Additionally, for a side to be a separator, the other two vertices MUST be on
  //the OTHER side from those of the other polygon
  for (int ai=0; ai<4; ++ai) {
    const pair<float,float> &first=a.vertices[ai];
    const pair<float,float> &second=a.vertices[(ai-1) & 3];
    const pair<float,float> edge=second-first;
    pair<float,float> rotated;
    rotated.first  = -edge.second;
    rotated.second = edge.first;
    const pair<float,float>&  middle(second);
    //middle.first=(first.first + second.first)/2;
    //middle.second=(first.second + second.second)/2;

    DotSide side=dot(rotated, b.vertices[0], middle);
    bool couldBeSeparator=true;
    for (int bi=1; bi<4 && couldBeSeparator; ++bi) {
      DotSide bside=dot(rotated, b.vertices[bi], middle);
      if (side==Collinear) side=bside;
      else if (bside!=Collinear && bside!=side)
        couldBeSeparator=false;
    }

    if (!couldBeSeparator) continue;

    //If all other point were collinear, we have a problem
    #ifdef DEBUG
    if (side==Collinear) {
      //cerr << "Problem with rectanglesCollideImpl(): All opposing vertices collinear!" << endl;
      //Benefit of doubt
      return false;
    }
    #endif

    //Check for opposite side of other two vertices
    //If both are on the same side as the other rectangle,
    //this side is meaningless
    bool oppositesOnSameSide=true;
    for (int avo=1; avo<3 && oppositesOnSameSide; ++avo) {
      int avi=(ai+avo)&3;
      DotSide aside=dot(rotated, a.vertices[avi], middle);
      if (aside!=side)
        oppositesOnSameSide=false;
    }
    //We know couldBeSeparator is true at this point
    if (!oppositesOnSameSide /*&& couldBeSeparator*/) return false;
  }
  return true;
}

/* Searches the two collision trees for collisions. If acc is NULL,
 * immediately returns true on base-level collision; continues
 * otherwise, adding to acc.
 */
static bool walkCollisionTree(CollisionRectangle*const* av, unsigned ac,
                              CollisionRectangle*const* bv, unsigned bc,
                              vector<const CollisionRectangle*>* acc) {
  for (unsigned ai = 0; ai < ac; ++ai) for (unsigned bi = 0; bi < bc; ++bi) {
    //Don't bother with a full check if there is no way the rectangles
    //could overlap
    #define av0x av[ai]->vertices[0].first
    #define av0y av[ai]->vertices[0].second
    #define bv0x bv[bi]->vertices[0].first
    #define bv0y bv[bi]->vertices[0].second
    #define avr av[ai]->radius
    #define bvr bv[bi]->radius
    if (((av0x > bv0x-bvr-avr && av0x <= bv0x+bvr+avr)
    ||   (bv0x > av0x-avr-bvr && bv0x <= av0x+avr+bvr))
    &&  ((av0y > bv0y-bvr-avr && av0y <= bv0y+bvr+avr)
    ||   (bv0y > av0y-avr-bvr && bv0y <= av0y+avr+bvr))) {
      if (rectanglesCollide(*av[ai], *bv[bi])) {
        if (av[ai]->recurse || bv[bi]->recurse) {
          //We need to check a finer level
          CollisionRectangle*const* asv, *const* bsv;
          unsigned asc, bsc;
          if (av[ai]->recurse)
            av[ai]->recurse(av[ai], asv, asc);
          else
            asv = av, asc = ac;
          if (bv[bi]->recurse)
            bv[bi]->recurse(bv[bi], bsv, bsc);
          else
            bsv = bv, bsc = bc;
          if (walkCollisionTree(asv, asc, bsv, bsc, acc))
            return true; //Subcall returned true, propagate up
        } else if (acc) {
          acc->push_back(av[ai]); //Collision, accumulate
        } else {
          return true; //Collision without accumulator, return immediately
        }
      }
    }

    #undef bvr
    #undef avr
    #undef bv0y
    #undef bv0x
    #undef av0y
    #undef av0x
  }

  return false; //No collisions, or solely accumulation
}

bool objectsCollide(GameObject* a, GameObject* b) noth {
  const vector<CollisionRectangle*>* avp(a->getCollisionBounds()),
                                   * bvp(b->getCollisionBounds());
  const vector<CollisionRectangle*>& av(*avp), &bv(*bvp);
  return walkCollisionTree(&av[0], av.size(), &bv[0], bv.size(), NULL);
}

void accumulateCollision(GameObject* a, GameObject* b, vector<const CollisionRectangle*>& acc) noth {
  const vector<CollisionRectangle*>* avp(a->getCollisionBounds()),
                                   * bvp(b->getCollisionBounds());
  const vector<CollisionRectangle*>& av(*avp), &bv(*bvp);
  walkCollisionTree(&av[0], av.size(), &bv[0], bv.size(), &acc);
}

pair<float, float> closestEdgePoint(const CollisionRectangle& rect, const pair<float,float>& c) noth {
  //First, find the distances to each vertex. The closest edge is the one composed of the two
  //nearest contiguous vertices
  float distances[4];
  for (int i=0; i<4; ++i) {
    float dx=rect.vertices[i].first-c.first,
          dy=rect.vertices[i].second-c.second;
    distances[i]=dx*dx + dy*dy;
  }
  float minDist=distances[0];
  int minVert=0;
  for (int i=1; i<4; ++i) {
    float vertD=distances[i];
    if (vertD<minDist) {
      minVert=i;
      minDist=vertD;
    }
  }

  int secondVert = (distances[ (minVert+1) & 3 ] < distances[ (minVert-1) & 3 ] ?
                    ((minVert+1) & 3) : ((minVert-1) & 3));
  const pair<float,float> &a(rect.vertices[minVert]),
                          &b(rect.vertices[secondVert]);
  const pair<float,float> ac(a-c), ab(a-b);
  float abdx=ab.first, abdy=ab.second;
  float r = dot(ac, ab) / (abdx*abdx + abdy*abdy);
  //R<=0 Use A
  //R>=0 Use B
  //Else Use point between A and B
  //If R becomes NaN, just return A
  if (r<=0)      return a;
  else if (r!=r) return a;
  else if (r>=1) return b;
  else           return make_pair(a.first  - r*ab.first,  //minus because we should use BA for this
                                  a.second - r*ab.second);
}
