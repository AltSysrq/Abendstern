/* "Texture" frag shader for shields. */
#version 130

uniform vec4 colour;
smooth in vec2 varyingTexCoord;
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
  /* Get the distance, first transforming the texture
   * coords from [0,0]x[1,1] to [-1,-1]x[+1,+1].
   */
  vec2 r = varyingTexCoord*2 - vec2(1,1);
  float distSq = r.x*r.x + r.y*r.y;
  if (distSq > 1) discard;
  distSq = cos(sqrt(distSq)*3.14159*4);
  distSq *= distSq;
  dst = huerot(colour, distSq);
  dst.a *= distSq;
}
