#version 120
/* Null vertex shader. Returns the vertex unmodified. */

attribute vec2 vertex;

void main(void) {
  gl_Position.xy=vertex;
  gl_Position.z=0;
  gl_Position.w=1;
}
