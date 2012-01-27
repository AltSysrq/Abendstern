/* Basic vertex shader. It transforms the input vertex
 * according to the provided matrix, and interpolates
 * colour by vertices.
 * LSD modification.
 */
#version 130

in vec4 vertex;
in vec4 colour;
smooth out vec4 varyingColour;
smooth out vec4 position;
flat out float transsum;
uniform mat4 transform;

void main(void) {
  varyingColour=colour;
  gl_Position = vertex*transform;
  position = vertex*transform;
  float sum = 0;
  for (int i=0; i<4; ++i) for (int j=0; j<4; ++j)
    sum += transform[i][j];
  transsum = sum;
}
