/* Quick fragment shader. Draws a single, uniform colour. */

#version 130

uniform vec4 colour;
out vec4 dst;
smooth in vec4 position;
flat in float transsum;

vec4 huerot(vec4 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  return vec4(v.r*max(0,cos(r)) + v.g*max(0,cos(r+t)) + v.b*max(0,cos(r-t)),
              v.g*max(0,cos(r)) + v.b*max(0,cos(r+t)) + v.r*max(0,cos(r-t)),
              v.b*max(0,cos(r)) + v.r*max(0,cos(r+t)) + v.g*max(0,cos(r-t)),
              v.a);
}

void main(void) {
  dst=huerot(colour, position.x+cos(5*position.y)+sin(10*position.x*position.y*transsum));
}
