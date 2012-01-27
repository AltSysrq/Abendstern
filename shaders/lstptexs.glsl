/* Stensiling texture stiple frag shader. */
#version 130

smooth in vec2 varyingTexCoords;
smooth in vec2 screenCoord;

uniform int screenW, screenH;
uniform sampler2D colourMap;
uniform vec4 modColour;

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
  int sx = int(floor((screenCoord.x+1)/2*screenW));
  int sy = int(floor((screenCoord.y+1)/2*screenH));
  if (sx%2 == sy%2) discard;
  float a = texture(colourMap, varyingTexCoords).a;
  if (a <= 0.01) discard;
  else dst=huerot(a*modColour, screenCoord.x*2.5 + cos(screenCoord.y*2)*2);
}
