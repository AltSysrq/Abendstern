#version 120
/* Frag shader to be used for energy charges.
 * This stacks on top of the basic texture
 * vertex shader, and uses the texture
 * coordinates to determine distance from the
 * centre.
 */

uniform vec4 colour;
varying in vec4 varyingTexCoord;
varying out vec4 dst;

void main(void) {
  dst = colour;
  dst.a *= 1-sqrt(varyingTexCoord.x*varyingTexCoord.x + varyingTexCoord.y*varyingTexCoord.y);
}
