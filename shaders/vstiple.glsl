/* Vertex shader for use with stipling fragment shaders.
 * This version does not work with textures.
 */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

uniform mat4 transform;
in vec4 vertex;
varying out vec2 screenCoord;

void main(void) {
  vec4 dst = vertex*transform;
  screenCoord = dst.xy;
  gl_Position = dst;
}
