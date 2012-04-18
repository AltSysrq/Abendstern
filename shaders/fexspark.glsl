#version 120
/* Explosion frag shader. Similar to sparkle, but time-independant,
 * resulting in an effect similar to the old explosions.
 * This version has a 256x256 pixel resolution.
 */

uniform int id;
uniform float elapsedTime;
uniform vec4 colour;
uniform float density;

/* varying */ in vec2 expCoord;

varying out vec4 dst;

/* Copied from:
 * http://lumina.sourceforge.net/Tutorials/Noise.html
 *
 * Return a PRN based on the given vector. Return value is
 * in the range [0,1].
 */
float rand(vec2 co) {
  return fract(sin(dot(co.xy, vec2(12.9898,78.233)))*43758.5453);
}

void main(void) {
  //Drop immediately if outside circle
  float distSq = expCoord.x*expCoord.x + expCoord.y*expCoord.y;
  if (distSq > 1) discard;
  else {
    float dist = sqrt(distSq);
    //Limit resolution to 256x256 so changes in camera do not
    //create sparkle
    vec2 rndEC = vec2(floor(128*(1+expCoord.x)), floor(128*(1+expCoord.y)));
    if (dist * (rand(id*rndEC)) > density) discard;
    else dst = colour;
  }
}
