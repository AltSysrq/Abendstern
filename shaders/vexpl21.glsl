#version 120
/* Open GL 2.1 vertex shader for explosions. */

in vec2 vertex;
uniform mat4 transform;
uniform float elapsedTime, sizeAt1Sec;
uniform float ex, ey;
varying out vec2 expCoord;

void main(void) {
  expCoord = vertex;
  vec4 v = vec4(0,0,0,1);
  v.xy = vec2(ex,ey) + (vertex*elapsedTime*sizeAt1Sec/2);
  gl_Position = v*transform;
}
