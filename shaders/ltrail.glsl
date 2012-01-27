/* Frag shader for trail. Use 1-abs(dist) to multiply
 * colour, and just use that.
 */

#version 150

smooth in vec4 varyingColour;
smooth in float dist;
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
  dst = huerot(varyingColour * (1-abs(dist)), dist);
}
