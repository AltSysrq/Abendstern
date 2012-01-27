/* Vertex shader for nebula-object-update computation. Transforms
 * input object to nebula coordinates and copies other information
 * to other variables so the geometry and fragment shaders can
 * access it.
 */
#version 150

const float numSimScreens = 8; //MUST be same as in C++
const float simSideSz = 8*128;
const float pointsPerScreen = 128;

uniform vec2 botleft;
in vec2 position,velocity;

flat out vec2 gpos,gvel;

void main(void) {
  gpos = (position-botleft)*pointsPerScreen; //Transform to 0..1 texture coords, then to pixel
  gvel = velocity;
  gl_Position.xy = gpos/simSideSz*2-1;
  gl_Position.z=0;
  gl_Position.w=1;
}
