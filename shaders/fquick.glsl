/* Quick fragment shader. Draws a single, uniform colour. */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

uniform vec4 colour;
varying out vec4 dst;

void main(void) {
  dst=colour;
}
