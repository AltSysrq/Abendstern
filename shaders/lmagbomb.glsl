/* Fragment shader for MagnetoBomb and the like.
 * Use with vbastex, as this computes the bomb's
 * graphic from the "texture coordinates".
 */
#version 130

uniform vec4 modColour;
uniform float rotation;
smooth in vec2 varyingTexCoord;

out vec4 dst;

float sq(float f) { return f*f; }
vec4 lumrot(vec4 v, float off) {
  float lum = sqrt(v.r*v.r +v.g*v.g + v.b*v.b);
  float th = (lum+off)*2*3.14159;
    return vec4(sq(cos(th)), sq(cos(th+3.14159/3)), sq(cos(th+2*3.14159/3)), v.a);
}

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
    dst.rgb = lumrot(modColour, distSq).rgb;
    dst.a = modColour.a * (1-distSq) * sin(angle*10)*sin(angle*10);
  }
}
