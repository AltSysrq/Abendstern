#version 120
/* "Texture" frag shader for shields. */

uniform vec4 colour;
varying in vec2 varyingTexCoord;
varying out vec4 dst;

void main(void) {
  /* Get the distance, first transforming the texture
   * coords from [0,0]x[1,1] to [-1,-1]x[+1,+1].
   */
  vec2 r = varyingTexCoord*2 - vec2(1,1);
  float distSq = r.x*r.x + r.y*r.y;
  if (distSq > 1) discard;
  dst = colour;
  dst.a *= distSq;
}
