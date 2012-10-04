#version 120
/* Same as vstiple, but works with textures. */

uniform mat4 transform;
attribute vec4 vertex;
attribute vec2 texCoord;
varying /* out*/ vec2 varyingTexCoord;
varying /* out */ vec2 screenCoord;

void main(void) {
  varyingTexCoord=texCoord;
  vec4 dst = vertex*transform;
  screenCoord = dst.xy;
  gl_Position = dst;
}
