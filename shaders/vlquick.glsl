/* Fast vertex shader. Only transforms vertices; supports
 * no other attributes.
 * LSD modification.
 */
#version 130

in vec2 vertex;
uniform mat4 transform;
smooth out vec4 position;
flat out float transsum;

void main(void) {
  vec4 v=vec4(vertex.x,vertex.y,0,1);
  gl_Position = v*transform;
  position = v*transform;
  float sum = 0;
  for (int i=0; i<4; ++i) for (int j=0; j<4; ++j)
    sum += transform[i][j];
  transsum = sum;
}
