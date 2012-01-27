/* Frag shader that discards everything. */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

varying out vec4 dst;

void main(void) {
  discard;
}
