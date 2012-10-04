#version 120
/* Missile frag shader, for use with the square graphic
 * and the vbastex vert shader.
 */

uniform float green;

varying /* in */ vec2 varyingTexCoord;
#define dst gl_FragColor

void main(void) {
  vec2 v = varyingTexCoord*2 - vec2(1,1);
  float distSq = v.x*v.x + v.y*v.y;
  if (distSq > 1) discard;

  dst = vec4(1.0, green + (1.0-green)*(1.0-distSq), 1.0-distSq, 1.0);
}
