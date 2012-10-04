#version 120
/* Vertex shader for light trail in 2.1 mode.
 * Since it does not have access to adjacent vertices,
 * it assumes that the velocity is an appropriate direction
 * for the trail, which may not be the case.
 */

attribute float ix;
attribute vec2 vertex;
attribute vec2 velocity;
attribute float creation;
attribute float isolation;
attribute float expansion;

uniform vec4 baseColour;
uniform vec4 colourFade;
uniform float currentTime;
uniform float baseWidth;
uniform mat4 transform;

varying /* out */ float dist;
varying /* out */ vec4 varyingColour;

void main(void) {
  dist = ix;
  float et = currentTime - creation;

  vec2 expansionDirection = normalize(vec2(velocity.y,-velocity.x));

  vec2 pos = vertex + velocity*et;
  float vertexWidth = expansion*et + baseWidth;
  varyingColour = (baseColour + colourFade*et)*isolation;

  pos += vertexWidth * expansionDirection * ix;
  gl_Position = vec4(pos.x,pos.y,0,1)*transform;
}