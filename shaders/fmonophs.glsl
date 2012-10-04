#version 120
/* Frag shader for Monophasic Energy Pulse. */

uniform vec3 baseColour;
uniform float visibility;

varying /* in */ vec2 varyingTexCoord;

#define dst gl_FragColor

void main(void) {
  vec2 v = varyingTexCoord*2 - vec2(1,1);
  float distSq = v.x*v.x + v.y*v.y;
  if (distSq > 1) discard;

  dst.rgb = baseColour;
  dst.a = visibility*visibility*(1-distSq);
}
