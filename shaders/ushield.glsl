/* "Texture" frag shader for shields. */
#version 130

uniform vec4 colour;
smooth in vec2 varyingTexCoord;
out vec4 dst;

void main(void) {
  /* Get the distance, first transforming the texture
   * coords from [0,0]x[1,1] to [-1,-1]x[+1,+1].
   */
  vec2 r = varyingTexCoord*2 - vec2(1,1);
  float distSq = r.x*r.x + r.y*r.y;
  if (distSq > 1 || distSq < 0.7) discard;
  dst.rgb = colour.rgb * colour.a;
  dst.a = 1;
}
