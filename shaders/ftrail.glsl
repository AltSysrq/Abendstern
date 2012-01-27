/* Frag shader for trail. Use 1-abs(dist) to multiply
 * colour, and just use that.
 */
#version 150

smooth in vec4 varyingColour;
smooth in float dist;
out vec4 dst;

void main(void) {
  dst = varyingColour * (1-abs(dist));
}
