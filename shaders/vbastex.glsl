/* Basic vertex shader for use with simple texture functions.
 * It simply passes the texture coordinates to the frag shader,
 * and translates the vertices appropriately.
 */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

uniform mat4 transform;
in vec2 texCoord;
in vec4 vertex;
varying out vec2 varyingTexCoord;

void main(void) {
  varyingTexCoord.st=texCoord;
  gl_Position=vertex*transform;
}

