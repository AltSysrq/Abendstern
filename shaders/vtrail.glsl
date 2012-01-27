/* Vertex shader for trails. It does not do actual transformation,
 * but calculates updates and provides additional information to
 * the geometry shader.
 */
#version 150

in vec2 vertex;
in vec2 velocity;
in float creation;
in float isolation;
in float expansion;

uniform vec4 baseColour;
uniform vec4 colourFade;
uniform float currentTime;
uniform float baseWidth;

flat out float vertexWidth;
flat out vec4 vertexColour;
smooth out vec4 varyingColour;

void main(void) {
  float et = currentTime - creation;

  vec2 pos = vertex + velocity*et;
  gl_Position = vec4(pos.x, pos.y, 0, 1);
  vertexWidth = expansion*et + baseWidth;
  varyingColour = vertexColour = (baseColour + colourFade*et)*isolation;
}
