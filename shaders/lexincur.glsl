/* Explosion frag shader. Draws a sci-fi style "incursion" type explosion.
 * That is, a ring of high intensity with internal fluctuations, and a
 * fading centre glow.
 */
#version 150

uniform float elapsedTime;
uniform vec4 colour;

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
  float dist = length(expCoord);
  if (dist > 1) discard; //Outside of ring
  else if (dist < 0.5) {
    //Inner glow area, if still existent
    if (elapsedTime < 1) {
      dst = colour;
      dst.a *= (1-elapsedTime);
    } else discard;
  } else {
    float frontWeight = abs(cos(3.14159*2*(dist-0.5)));
    float angle = atan(expCoord.y, expCoord.x);
    float staticRingWeight = sin(angle*32)*sin(angle*32);
    float dynamicRingWeight = sin(elapsedTime*2+angle*32);
    float ringWeight = abs(staticRingWeight+dynamicRingWeight)/2;
    dst = huerot(colour,dist+elapsedTime);
    float mul = max(frontWeight, ringWeight);
    dst.a *= mul;
    //Discolour inner parts
    dst.b *= 1 - (1 - frontWeight)/2;
    dst.g *= 1 - (1 - frontWeight)/4;
  }
}
