/* Fragment shader for MagnetoBomb and the like.
 * Use with vbastex, as this computes the bomb's
 * graphic from the "texture coordinates".
 */
#version 130

uniform vec4 modColour;
uniform float rotation;
smooth in vec2 varyingTexCoord;

out vec4 dst;

void main(void) {
  float txx=(varyingTexCoord.x-0.5)*2;
  float txy=(varyingTexCoord.y-0.5)*2;
  float distSq = txx*txx + txy*txy;
  if (distSq > 1) discard;

  //Full brightness within the core
  if (distSq < 0.25*0.25) {
    dst.rgb = modColour.rgb;
    dst.a = 1;
  } else if (rotation < 1000) { //So rotation doesn't get dropped
    discard;
  }
}
