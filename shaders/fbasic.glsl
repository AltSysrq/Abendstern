/* Simple identity frag shader. */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

varying in vec4 varyingColour;
varying out vec4 dst;

void main(void) {
  dst = varyingColour;
}
