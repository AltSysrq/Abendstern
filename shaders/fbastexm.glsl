#version 120
/* Basic texture fragment shader. Multiplies a uniform colour
 * by the texture samples, emulating GL_MODULATE.
 */

varying /* in */ vec2 varyingTexCoord;
uniform sampler2D colourMap;
uniform vec4 modColour;
#define dst gl_FragColor

void main(void) {
  vec4 colour = texture2D(colourMap, varyingTexCoord);
  colour *= modColour;
  if (colour.a < 0.01) discard;
  dst = colour;
}
