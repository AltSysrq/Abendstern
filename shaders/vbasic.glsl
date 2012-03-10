#version 120
/* Basic vertex shader. It transforms the input vertex
 * according to the provided matrix, and interpolates
 * colour by vertices.
 */

in vec4 vertex;
in vec4 colour;
varying out vec4 varyingColour;
uniform mat4 transform;

void main(void) {
  varyingColour=colour;
  gl_Position = vertex*transform;
}
