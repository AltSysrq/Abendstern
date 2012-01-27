/* Explosion fragment shader for a simple explosion.
 * Color is blended out as distance from the source
 * increases.
 */
#version 130

uniform vec4 colour;
uniform float elapsedTime;
smooth in vec2 expCoord;

out vec4 dst;

vec4 huerot(vec4 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  return vec4(v.r*max(0,cos(r)) + v.g*max(0,cos(r+t)) + v.b*max(0,cos(r-t)),
              v.g*max(0,cos(r)) + v.b*max(0,cos(r+t)) + v.r*max(0,cos(r-t)),
              v.b*max(0,cos(r)) + v.r*max(0,cos(r+t)) + v.g*max(0,cos(r-t)),
              v.a);
}

void main(void) {
  //Don't waste time with a sqrt if we don't have to
  float distSq = expCoord.x*expCoord.x+expCoord.y*expCoord.y;
  if (distSq >= 1) discard;
  else dst = huerot(vec4(colour.r, colour.g, colour.b, colour.a * (1-sqrt(distSq))), elapsedTime*3);
}
