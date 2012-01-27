/* Vertex shader to pass information to the nebula-base-update
 * fragment shader.
 */

#version 150

/* The change in location of the bottom-left, in texture coordinates. */
uniform vec2 delta;
in vec2 position, texCoord;
smooth out vec2 pos;

void main(void) {
  pos = texCoord+delta;
  gl_Position = vec4(position.x, position.y, 0, 1);
}
