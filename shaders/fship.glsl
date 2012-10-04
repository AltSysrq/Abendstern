#version 120
/* Shader for ships in their final, texturized form.
 * Texture 0 is the ship itself, while texture 1 is the damage
 * texture. A uniform array specifying the pallet must be sent
 * for the higher half.
 */

uniform vec3 shipColour;
uniform float highColour[128*3];
uniform sampler2D mainTex, damTex;
uniform float cloak;
varying /* in */ vec2 varyingTexCoord;
#define dst gl_FragColor

void main(void) {
  vec4 colour=texture2D(mainTex, varyingTexCoord);
  int ix = int(colour.r*255);
  if (ix == 0) {
    discard;
  } else if (ix == 1) {
    dst = vec4(0,0,0,1);
  } else if (ix < 128) {
    dst.rgb = shipColour * ((ix-1)/127.0*0.7f + 0.3f);
  } else {
    dst.r = highColour[(ix-128)*3+0];
    dst.g = highColour[(ix-128)*3+1];
    dst.b = highColour[(ix-128)*3+2];
  }
  dst.a=cloak;
  dst.rgb *= texture2D(damTex, varyingTexCoord).rgb;
}
