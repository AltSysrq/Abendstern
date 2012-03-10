#version 120
/* Basic texture fragment shader. Multiplies a uniform colour
 * by the alpha component of the texture.
 */

varying in vec2 varyingTexCoord;
uniform sampler2D colourMap;
uniform vec4 modColour;
varying out vec4 dst;

void main(void) {
  float coloura = texture2D(colourMap, varyingTexCoord).a;
  if (coloura < 0.01) discard;
  dst.rgb = coloura*modColour.rgb;
  dst.a = coloura*modColour.a;
}
