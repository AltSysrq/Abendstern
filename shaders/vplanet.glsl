#version 120
/* Vertex shader for use with Planet.
 * Interpolates day/night. (day=1, night=0)
 */

uniform mat4 transform;
in vec2 texCoord;
in vec2 vertex;
in float isLeft;
uniform float dayNightLeft, dayNightRight;
varying out vec2 varyingTexCoord;
varying out float varyingDayNight;

void main(void) {
  varyingDayNight=(isLeft==1? dayNightLeft : dayNightRight);
  varyingTexCoord=texCoord;
  gl_Position = vec4(vertex.x, vertex.y, 0, 1)*transform;
}
