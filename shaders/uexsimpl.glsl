/* Explosion fragment shader for a simple explosion.
 * Color is blended out as distance from the source
 * increases.
 */
#version 130

uniform vec4 colour;
smooth in vec2 expCoord;

out vec4 dst;

void main(void) {
  //Don't waste time with a sqrt if we don't have to
  float distSq = expCoord.x*expCoord.x+expCoord.y*expCoord.y;
  if (distSq >= 1) discard;
  else {
    dst.rgb = vec3(colour.r*colour.a, colour.g*colour.a, colour.b*colour.a);
    dst.a=1;
  }
}
