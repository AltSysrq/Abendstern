#version 120
/* Frag shader for trail. Use 1-abs(dist) to multiply
 * colour, and just use that.
 */

/* varying */ in vec4 varyingColour;
/* varying */ in float dist;
varying out vec4 dst;

void main(void) {
  dst = varyingColour * (1-abs(dist));
}
