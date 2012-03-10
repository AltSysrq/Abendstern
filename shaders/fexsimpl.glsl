#version 120
/* Explosion fragment shader for a simple explosion.
 * Color is blended out as distance from the source
 * increases.
 */

uniform vec4 colour;
varying in vec2 expCoord;

varying out vec4 dst;

void main(void) {
  //Don't waste time with a sqrt if we don't have to
  float distSq = expCoord.x*expCoord.x+expCoord.y*expCoord.y;
  if (distSq >= 1) discard;
  else dst = vec4(colour.r, colour.g, colour.b, colour.a * (1-sqrt(distSq)));
}
