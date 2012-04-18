#version 120
/* Frag shader for planet.
 * Takes day texture and night texture, mixes appropriately.
 */

uniform sampler2D dayTex, nightTex;
uniform vec4 glareColour;
/* varying */ in float varyingDayNight;
/* varying */ in vec2 varyingTexCoord;
varying out vec4 dst;

void main(void) {
  if (varyingDayNight == 0) {
    //Night only
    dst = texture2D(nightTex, varyingTexCoord) + glareColour;
  } else if (varyingDayNight == 1) {
    //Day only
    dst = texture2D(dayTex, varyingTexCoord);
  } else {
    //Mix
    dst = texture2D(dayTex, varyingTexCoord)*varyingDayNight
        +(texture2D(nightTex, varyingTexCoord)+glareColour)*(1-varyingDayNight);
  }
}
