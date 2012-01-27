/* Frag shader for nebula foreground */
#version 150

uniform vec3 colour;

const float maxViewDist = 8;

smooth in vec2 varyingTexCoord;
out vec4 dst;

void main(void) {
  vec2 p = varyingTexCoord*2-vec2(1,1);
  dst.a = min(1.0, sqrt(p.x*p.x + p.y*p.y)/maxViewDist);
  dst.rgb = colour;
}
