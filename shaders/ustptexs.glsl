/* Stensiling texture stiple frag shader. */
#version 130

smooth in vec2 varyingTexCoords;
smooth in vec2 screenCoord;

uniform int screenW, screenH;
uniform sampler2D colourMap;
uniform vec4 modColour;

out vec4 dst;

void main(void) {
  int sx = int(floor((screenCoord.x+1)/2*screenW));
  int sy = int(floor((screenCoord.y+1)/2*screenH));
  if (sx%2 == sy%2) discard;
  if (0.01 >= texture(colourMap, varyingTexCoords).a) discard;
  else dst.rgb=modColour.rgb;
  dst.a=1;
}
