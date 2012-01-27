/* Frag shader for planet.
 * Takes day texture and night texture, mixes appropriately.
 */
#version 130

uniform sampler2D dayTex, nightTex;
uniform vec4 glareColour;
smooth in float varyingDayNight;
smooth in vec2 varyingTexCoord;
out vec4 dst;

vec4 huerot(vec4 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  return vec4(v.r*max(0,cos(r)) + v.g*max(0,cos(r+t)) + v.b*max(0,cos(r-t)),
              v.g*max(0,cos(r)) + v.b*max(0,cos(r+t)) + v.r*max(0,cos(r-t)),
              v.b*max(0,cos(r)) + v.r*max(0,cos(r+t)) + v.g*max(0,cos(r-t)),
              v.a);
}

vec4 lumrot(vec4 v, float off) {
  float lum = sqrt(v.r*v.r +v.g*v.g + v.b*v.b);
  float th = (lum+off)*2*3.14159;
  return vec4(max(0,cos(th)), max(0,cos(th+2*3.14159/3)), max(0,cos(th+4*3.14159/3)), v.a);
}

void main(void) {
  if (varyingDayNight == 0) {
    //Night only
    dst = texture(nightTex, varyingTexCoord) + glareColour;
  } else if (varyingDayNight == 1) {
    //Day only
    dst = texture(dayTex, varyingTexCoord);
  } else {
    //Mix
    dst = texture(dayTex, varyingTexCoord)*varyingDayNight
        + (texture(nightTex, varyingTexCoord)+glareColour)*(1-varyingDayNight);
  }

  dst = (lumrot(dst, varyingTexCoord.s + varyingTexCoord.t)
       + 3*lumrot(dst, 0)
       + 5*huerot(dst, varyingTexCoord.s - 2*varyingTexCoord.t))/8;
}
