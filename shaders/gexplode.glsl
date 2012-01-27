/* Geom shader that takes a simple point and emits the necessary
 * vertices and other information for an Explosion.
 */
#version 150

layout (points) in;
layout (triangle_strip, max_vertices=4) out;

uniform mat4 transform;
uniform float elapsedTime, sizeAt1Sec;
uniform float ex, ey;
smooth out vec2 expCoord;

void main(void) {
  vec4 centre = vec4(ex,ey,0,1);
  float hsize=elapsedTime*sizeAt1Sec/2;

  expCoord = vec2(-1, +1);
  gl_Position = vec4(centre.x-hsize, centre.y+hsize, 0, 1)*transform;
  EmitVertex();
  expCoord = vec2(-1, -1);
  gl_Position = vec4(centre.x-hsize, centre.y-hsize, 0, 1)*transform;
  EmitVertex();
  expCoord = vec2(+1, +1);
  gl_Position = vec4( centre.x+hsize, centre.y+hsize, 0, 1)*transform;
  EmitVertex();
  expCoord = vec2(+1, -1);
  gl_Position = vec4(centre.x+hsize, centre.y-hsize, 0, 1)*transform;
  EmitVertex();
  EndPrimitive();
}
