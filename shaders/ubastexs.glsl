/* Basic texture fragment shader. Multiplies a uniform colour
 * by the alpha component of the texture.
 */
#version 130

smooth in vec2 varyingTexCoord;
uniform sampler2D colourMap;
uniform vec4 modColour;
out vec4 dst;

void main(void) {
  float coloura = texture(colourMap, varyingTexCoord).a;
  if (coloura < 0.01) discard;
  dst.rgb = coloura*modColour.rgb;
  dst.a=1;
}
