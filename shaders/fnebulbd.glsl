/* Frag shader for nebula background. */
#version 150

uniform vec3 colour;
uniform sampler2D d;

const float simSideSz = 8*128, px = 1.0/simSideSz;

varying in vec2 varyingTexCoord;
varying out vec4 dst;

void main(void) {
  float offs[3];
  offs[0] = -px;
  offs[1] = 0;
  offs[2] = +px;
  dst.a=1;
  for (int x=0; x<3; ++x) for (int y=0; y<3; ++y)
    dst.rgb += colour*texture(d,vec2(varyingTexCoord.x+offs[x],1-varyingTexCoord.y+offs[y])).r;
//  dst.rgb = colour*texture(d,vec2(varyingTexCoord.x,1-varyingTexCoord.y)).r;
  dst.rgb *= 1/9.0;
  //dst.rgb = texture(d,vec2(varyingTexCoord.x,1-varyingTexCoord.y)).rgb*vec3(1,10000000,10000000);
}
