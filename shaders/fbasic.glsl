#version 120

/* Simple identity frag shader. */

varying /* in */ vec4 varyingColour;
#define dst gl_FragColor

void main(void) {
  dst = varyingColour;
}
