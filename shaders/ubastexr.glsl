/* Basic texture fragment shader, replace mode. */
#version 130

smooth in vec2 varyingTexCoord;
uniform sampler2D colourMap;
out vec4 dst;

void main(void) {
  vec4 colour = texture(colourMap, varyingTexCoord);
  if (colour.a < 0.01) discard;
  dst.rgb = colour.rgb;
  dst.a=1;
}
