#version 120
/* Basic texture fragment shader, replace mode. */

varying in vec2 varyingTexCoord;
uniform sampler2D colourMap;
varying out vec4 dst;

void main(void) {
  vec4 colour = texture2D(colourMap, varyingTexCoord);
  if (colour.a < 0.01) discard;
  dst = colour;
}
