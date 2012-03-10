#version 120
/* Stensiling texture stiple frag shader. */

varying in vec2 varyingTexCoords;
varying in vec2 screenCoord;

uniform int screenW, screenH;
uniform sampler2D colourMap;
uniform vec4 modColour;

varying out vec4 dst;

//The % operator is non-standard in GLSL 1.2 and
//illegal in GLSL ES 1.0 (why?).
//Emulate it
bool bit0(int i) {
  return i/2 == (i+1)/2;
}

void main(void) {
  int sx = int(floor((screenCoord.x+1)/2*screenW));
  int sy = int(floor((screenCoord.y+1)/2*screenH));
  if (bit0(sx) == bit0(sy)) discard;
  float a = texture2D(colourMaP, varyingTexCoords).a;
  if (a <= 0.01) discard;
  else dst=a*modColour;
}
