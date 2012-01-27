/* Advanced shader for ships in their final, texturized form.
 * Texture 0 is the ship itself, while texture 1 is the damage
 * texture. A uniform array specifying the pallet must be sent
 * for the higher half.
 */
#version 150

uniform vec3 shipColour;
uniform float highColour[128*3];
uniform sampler2D mainTex, damTex;
uniform float cloak;
smooth in vec2 varyingTexCoord;
out vec4 dst;

float sq(float f) { return f*f; }
vec3 lumrot(vec3 v, float off) {
  float lum = sqrt(v.r*v.r +v.g*v.g + v.b*v.b);
  float th = (lum+off)*2*3.14159;
    return vec3(sq(cos(th)), sq(cos(th+3.14159/3)), sq(cos(th+2*3.14159/3)));
}

vec3 huerot(vec3 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  return vec3(v.r*max(0,cos(r)) + v.g*max(0,cos(r+t)) + v.b*max(0,cos(r-t)),
              v.g*max(0,cos(r)) + v.b*max(0,cos(r+t)) + v.r*max(0,cos(r-t)),
              v.b*max(0,cos(r)) + v.r*max(0,cos(r+t)) + v.g*max(0,cos(r-t)));
}

void main(void) {
  vec4 colour=texture(mainTex, varyingTexCoord);
  int ix = int(colour.r*255);
  if (ix == 0) {
    discard;
  } else if (ix == 1) {
    dst = vec4(0,0,0,1);
  } else if (ix < 128) {
    dst.rgb = lumrot(shipColour, float(ix)/53);
  } else {
    dst.r = highColour[(ix-128)*3+0];
    dst.g = highColour[(ix-128)*3+1];
    dst.b = highColour[(ix-128)*3+2];
  }
  dst.a=cloak;
  dst.rgb = texture(damTex, varyingTexCoord).r
          * huerot(dst.rgb, highColour[1]+
                   varyingTexCoord.s+0.35*sin(2*3.14159*varyingTexCoord.t));
}
