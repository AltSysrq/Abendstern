/* Explosion frag shader. Similar to sparkle, but time-independant,
 * resulting in an effect similar to the old explosions.
 * This version has a 256x256 pixel resolution.
 */
#version 150

uniform int id;
uniform float elapsedTime;
uniform vec4 colour;
uniform float density;

smooth in vec2 expCoord;

out vec4 dst;

/* Copied from:
 * http://lumina.sourceforge.net/Tutorials/Noise.html
 *
 * Return a PRN based on the given vector. Return value is
 * in the range [0,1].
 */
float rand(vec2 co) {
  return fract(sin(dot(co.xy, vec2(12.9898,78.233)))*43758.5453);
}

vec4 huerot(vec4 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  return vec4(v.r*max(0,cos(r)) + v.g*max(0,cos(r+t)) + v.b*max(0,cos(r-t)),
              v.g*max(0,cos(r)) + v.b*max(0,cos(r+t)) + v.r*max(0,cos(r-t)),
              v.b*max(0,cos(r)) + v.r*max(0,cos(r+t)) + v.g*max(0,cos(r-t)),
              v.a);
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
    else dst = huerot(colour, dist+elapsedTime*3);
  }
}
