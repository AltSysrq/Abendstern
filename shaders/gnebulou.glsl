/* Geometry shader to expand one point into four pixel-alligned
 * points for the nebula-object-update pass. Discards points on
 * or outside the border.
 * This shader works only with pixel coordinates.
 */

#version 150
layout (points) in;
layout (points, max_vertices=4) out;

flat in vec2 gpos[], gvel[];
flat out vec2 pos, vel, coord;

//MUST be the same as in C++
const float simSideSz = 8*128;
const float px = 2.0/simSideSz;

void handle(vec2 v, float ox, float oy) {
  v.x += ox;
  v.y += oy;
  coord = v;
  pos = gpos[0];
  vel = gvel[0];
  gl_Position.xy = coord/simSideSz*2 - vec2(1,1);
  gl_Position.z = 0;
  gl_Position.w = 1;
  EmitVertex();
}

void main(void) {
  vec2 v = (gl_in[0].gl_Position.xy+vec2(1,1))/2*simSideSz;
  v.x = floor(v.x);
  v.y = floor(v.y);
  handle(v, 0, 0);
  handle(v, 1, 0);
  handle(v, 0, 1);
  handle(v, 1, 1);
}
