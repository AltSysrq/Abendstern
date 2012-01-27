/* Basic texture fragment shader. Multiplies a uniform colour
 * by the texture samples, emulating GL_MODULATE.
 * Ugly version.
 */
#version 130

smooth in vec2 varyingTexCoord;
uniform sampler2D colourMap;
uniform vec4 modColour;
out vec4 dst;

void main(void) {
  vec4 colour = texture(colourMap, varyingTexCoord);
  colour *= modColour;
  if (colour.a < 0.01) discard;
  dst.rgb = colour.rgb*modColour.a;
  dst.a=1;
}
