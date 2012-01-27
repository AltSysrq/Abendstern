/* Simple colour-only stipler. */
#version 130

smooth in vec4 varyingColour;
smooth in vec2 screenCoord;

uniform int screenSz;

out vec4 dst;

void main(void) {
  int sx = int(floor((screenCoord.x+1)/2*screenW));
  int sy = int(floor((screenCoord.y+1)/2*screenH));
  if (sx%2 == sy%2) discard;
  else dst.rgb=varyingColour.rgb;
  dst.a=1;
}
