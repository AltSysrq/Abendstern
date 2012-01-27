/* Null vertex shader. Returns the vertex unmodified. */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

in vec2 vertex;

void main(void) {
  gl_Position.xy=vertex;
  gl_Position.z=0;
  gl_Position.w=1;
}
