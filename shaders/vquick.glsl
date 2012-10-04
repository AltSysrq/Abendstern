#version 120
/* Fast vertex shader. Only transforms vertices; supports
 * no other attributes.
 */

attribute vec2 vertex;
uniform mat4 transform;

void main(void) {
  vec4 v=vec4(vertex.x,vertex.y,0,1);
  gl_Position = v*transform;
}
