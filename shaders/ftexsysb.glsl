#version 120
/* Frag shader for drawing system textures.
 * It works like a normal textured frag shader, except that
 * it ignores alpha and discards fragments which have an R
 * of less than 1/512 (it also assumes that the input is
 * greyscale, and therefore also ignores G and B).
 */

/* varying */ in vec2 varyingTexCoord;
uniform sampler2D colourMap;

varying out vec4 dst;

void main(void) {
  float lum=texture2D(colourMap, varyingTexCoord).r;
  if (lum < 1/512.0) discard;

  dst.r=lum;
  dst.g=lum;
  dst.b=lum;
  dst.a=1;
}
