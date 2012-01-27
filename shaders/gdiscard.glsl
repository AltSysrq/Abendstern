/* Geom shader that discards everything. It performs a
 * meaningless test against the transform so that that
 * uniform is not dropped by the compiler.
 */
#version 150

layout (points) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 transform;

void main(void) {
  if (transform[0][0] > 999999) {
    for (int i=0; i<3; ++i) {
      gl_Position = gl_in[0].gl_Position;
      EmitVertex();
    }
  }
}
