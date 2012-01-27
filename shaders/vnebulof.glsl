/* Vertex shader for nebula-object-update computation. Transforms
 * input object to nebula coordinates and calculates force as feedback.
 */
#version 150 core

const float numSimScreens = 8; //MUST be same as in C++
const float simSideSz = 8*128;
const float pointsPerScreen = 128;
const float pxs = 1.0/pointsPerScreen;

uniform vec2 botleft, delta;
uniform sampler2D d;
uniform float mass, forceMul;
in vec2 position,velocity;

out vec2 force;

void main(void) {
  vec2 gpos = (position-botleft)*pointsPerScreen; //Transform to 0..1 texture coords, then to pixel
  vec4 dat = texture(d, gpos/simSideSz + delta);
  force = forceMul * mass * dat.r * (dat.gb/pointsPerScreen-velocity);
}
