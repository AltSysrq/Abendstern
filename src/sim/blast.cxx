/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/sim/blast.hxx
 */

#include <cmath>
#include <iostream>

#include "blast.hxx"
#include "collision.hxx"
using namespace std;

Blast::Blast(GameField* field, unsigned b, float _x, float _y, float _falloff,
             float _strength, bool _direct, float _size,
             bool addDecorativeCopy, bool decorative, bool cd)
: GameObject(field, _x, _y),
  falloff(_falloff), strength(_strength), size(_size),
  isAlive(true), direct(_direct), causeDamage(cd),
  blame(b)
{
  //We double the size because movement (but non-damaging) impact extends twice as far
  //See handling in ship.cxx
  rect.vertices[0].first =x-falloff*2-size;
  rect.vertices[0].second=y-falloff*2-size;
  rect.vertices[1].first =x-falloff*2-size;
  rect.vertices[1].second=y+falloff*2+size;
  rect.vertices[2].first =x+falloff*2+size;
  rect.vertices[2].second=y+falloff*2+size;
  rect.vertices[3].first =x+falloff*2+size;
  rect.vertices[3].second=y-falloff*2-size;
  rect.radius=sqrt(2.0f)*2*(falloff*2+size);
  collisionBounds.push_back(&rect);

  //Decorative copy
  if (addDecorativeCopy) field->inject(new Blast(this));
  if (decorative) {
    this->decorative=decorative;
    okToDecorate();
  }
}

Blast::Blast(const Blast* that)
: GameObject(that->field, that->x, that->y),
  falloff(that->falloff), strength(that->strength), size(that->size),
  rect(that->rect), isAlive(true), direct(that->direct), causeDamage(true),
  blame(that->blame)
{
  decorative=true;
  rect.radius=sqrt(2.0f)*2*(falloff*2+size);
  collisionBounds.push_back(&rect);
  okToDecorate();
}

Blast::Blast(const Blast* blast, bool _causeDamage)
: GameObject(blast->field, blast->x, blast->y),
  falloff(blast->falloff), strength(blast->strength), size(blast->size),
  rect(blast->rect), isAlive(true), direct(blast->direct), causeDamage(_causeDamage),
  blame(blast->blame)
{
  rect.radius=sqrt(2.0f)*2*(falloff*2+size);
  collisionBounds.push_back(&rect);
}

bool Blast::update(float e) noth {
  if (isAlive) {
    isAlive=false;
    return true;
  }
  return false;
}

void Blast::draw() noth {
}

CollisionResult Blast::checkCollision(GameObject* o) noth {
  if (o->isDecorative() != this->isDecorative()) return NoCollision;

  if (o->isCollideable())
    return (objectsCollide(this, o)? YesCollision : NoCollision);
  else return UnlikelyCollision;
}

float Blast::getRadius() const noth {
  return falloff;
}

float Blast::getFalloff() const noth {
  return falloff;
}

float Blast::getStrength() const noth {
  return strength;
}

float Blast::getStrength(float distance) const noth {
  distance=fabs(distance);
  if (distance>size) distance-=size;
  else distance=0;
  if      (distance>falloff) return 0;
  else {
    float mult=1-distance/falloff;
    return strength*mult*mult;
  }
}

float Blast::getStrength(const GameObject* go) const noth {
  float dx=go->getX()-x, dy=go->getY()-y;
  return getStrength(sqrt(dx*dx + dy*dy));
}

bool Blast::isDirect() const noth {
  return direct;
}

bool Blast::causesDamage() const noth {
  return causeDamage;
}
