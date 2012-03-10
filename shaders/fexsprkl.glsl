#version 120
/* An explosion frag shader that makes a "sparkling" explosion.
 * It implements its own PRNG.
 */

uniform int id;
uniform float elapsedTime;
uniform vec4 colour;
uniform float density;

varying in vec2 expCoord;

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
    //Sparkle changes 20 times per second
    float etKey = (mod(floor(elapsedTime*1000/100),16)+1) * 10;
    //Limit resolution to 256*256 so changes in camera do not
    //create sparkle
    vec2 rndEC = vec2(floor(128*(1+expCoord.x)), floor(128*(1+expCoord.y)));
    if (dist * (rand(etKey*id*rndEC)) > density*(1-dist)) discard;
    else dst = colour;
  }
}
