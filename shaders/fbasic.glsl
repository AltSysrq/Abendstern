#version 120

/* Simple identity frag shader. */

/* varying */ in vec4 varyingColour;
varying out vec4 dst;

void main(void) {
  dst = varyingColour;
}
