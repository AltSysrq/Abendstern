/* Frag shader to be used for energy charges.
 * This stacks on top of the basic texture
 * vertex shader, and uses the texture
 * coordinates to determine distance from the
 * centre.
 */
#version 130

uniform vec4 colour;
smooth in vec4 varyingTexCoord;
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
  dst = huerot(colour, varyingTexCoord.x);
}
