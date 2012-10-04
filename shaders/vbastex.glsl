#version 120
/* Basic vertex shader for use with simple texture functions.
 * It simply passes the texture coordinates to the frag shader,
 * and translates the vertices appropriately.
 */

uniform mat4 transform;
attribute vec2 texCoord;
attribute vec4 vertex;
varying /* out */ vec2 varyingTexCoord;

void main(void) {
  varyingTexCoord.st=texCoord;
  gl_Position=vertex*transform;
}

