/* Explosion frag shader. Similar to sparkle, but time-independant,
 * resulting in an effect similar to the old explosions.
 * This version has a 1024x1024 pixel resolution.
 *
 * This is entirely replaced by an LSD effect.
 */
#version 150

uniform int id;
uniform float elapsedTime;
uniform vec4 colour;
uniform float density;

smooth in vec2 expCoord;

out vec4 dst;

float sq(float f) { return f*f; }
vec4 lumrot(vec4 v, float off) {
  float lum = sqrt(v.r*v.r +v.g*v.g + v.b*v.b);
  float th = (lum+off)*2*3.14159;
    return vec4(sq(cos(th)), sq(cos(th+3.14159/3)), sq(cos(th+2*3.14159/3)), v.a);
}

void main(void) {
  //Drop immediately if outside circle
  float distSq = expCoord.x*expCoord.x + expCoord.y*expCoord.y;
  if (distSq > 1) discard;
  else {
    float dist = sqrt(distSq);
    dst = lumrot(colour, dist*elapsedTime*elapsedTime);
  }
}
