#version 120
/* Simple colour-only stipler. */

uniform vec4 colour;
varying in vec2 screenCoord;

uniform int screenW, screenH;

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
  //if (sx%2 == sy%2) discard;
  if (bit0(sx) == bit0(sy)) discard;
  else dst=colour;
}
