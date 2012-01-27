/* Same as vstiple, but works with textures. */
#ifndef GL_ES
#version 120
#else
#version 100
#endif

uniform mat4 transform;
in vec4 vertex;
in vec2 texCoord;
varying out vec2 varyingTexCoords;
varying out vec2 screenCoord;

void main(void) {
  varyingTexCoords=texCoord;
  vec4 dst = vertex*transform;
  screenCoord = dst.xy;
  gl_Position = dst;
}
