/* Simple identity frag shader, ugly version. */
#version 130

smooth in vec4 varyingColour;
out vec4 dst;

void main(void) {
  dst.rgb = varyingColour.rgb;
  dst.a=1;
}
