#version 120
/* Fragment shader for MagnetoBomb and the like.
 * Use with vbastex, as this computes the bomb's
 * graphic from the "texture coordinates".
 */

uniform vec4 modColour;
uniform float rotation;
varying in vec2 varyingTexCoord;

varying out vec4 dst;

void main(void) {
  float txx=(varyingTexCoord.x-0.5)*2;
  float txy=(varyingTexCoord.y-0.5)*2;
  float distSq = txx*txx + txy*txy;
  if (distSq > 1) discard;

  //Full brightness within the core
  if (distSq < 0.25*0.25) {
    dst = modColour;
  } else {
    //Adjust effective distance so it now varies from 0 to 1
    //[0.25,1]->[0,0.75]
    distSq -= 0.25;
    //[0,0.75]->[0,1]
    distSq /= 0.75;

    float angle = atan(txy,txx)+rotation;
    dst.rgb = modColour.rgb;
    dst.a = modColour.a * (1-distSq) * sin(angle*5)*sin(angle*5);
  }
}
