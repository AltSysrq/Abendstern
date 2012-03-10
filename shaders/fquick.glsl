#version 120
/* Quick fragment shader. Draws a single, uniform colour. */

uniform vec4 colour;
varying out vec4 dst;

void main(void) {
  dst=colour;
}
