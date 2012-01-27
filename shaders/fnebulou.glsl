/* Frag shader for nebula-object-update pass.
 * Make sure to disable writing to R for this shader to work.
 */
#version 150

const float simSideSz = 8*128; //MUST be the same as in C++

//Pos = position of element, vel = velocity thereof, coord = our pixel coordinate
flat in vec2 pos, vel, coord;
uniform sampler2D d; //Data from previous frame (since we can't safely access the current)
uniform vec2 delta;

out vec4 dst;

void main(void) {
  if (dot(coord-pos, vel - texture(d, coord/simSideSz+delta).gb) <= 0) discard; //No change
  vec2 cp = coord-pos;
  dst.gb = vec2(0.0,0.0);
    //vel;// * dot(cp, vel)/max(0.000001, sqrt(cp.x*cp.x+cp.y*cp.y)*sqrt(vel.x*vel.x + vel.y*vel.y));
  dst.a=1;
  dst.r=0.0;//delta.x*0.000000000000001*texture(d,vec2(0,0)).a;
}
