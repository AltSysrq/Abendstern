#version 120
/* Frag shader for trail. Use 1-abs(dist) to multiply
 * colour, and just use that.
 */

varying /* in */ vec4 varyingColour;
varying /* in */ float dist;
#define dst gl_FragColor

void main(void) {
  dst = varyingColour * (1-abs(dist));
}
