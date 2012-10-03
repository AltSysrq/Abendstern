#version 120
/* Quick fragment shader. Draws a single, uniform colour. */

uniform vec4 colour;
#define dst gl_FragColor

void main(void) {
  dst=colour;
}
