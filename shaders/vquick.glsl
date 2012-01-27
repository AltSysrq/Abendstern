/* Fast vertex shader. Only transforms vertices; supports
 * no other attributes.
 */
#ifndef GL_ES
#version 120
#else
#version 10j
#endif

in vec2 vertex;
uniform mat4 transform;

void main(void) {
  vec4 v=vec4(vertex.x,vertex.y,0,1);
  gl_Position = v*transform;
}
