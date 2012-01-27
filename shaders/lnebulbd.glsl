/* Frag shader for nebula background. */
#version 150

uniform vec3 colour;
uniform sampler2D d;

const float simSideSz = 8*128, px = 1.0/simSideSz;

smooth in vec2 varyingTexCoord;
out vec4 dst;

vec4 huerot(vec4 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  float s=t/2;
  return vec4(v.r*max(0,cos(r+0)) + v.g*max(0,cos(r+t+0)) + v.b*max(0,cos(r-t+0)),
              v.g*max(0,cos(r+s)) + v.b*max(0,cos(r+t+s)) + v.r*max(0,cos(r-t+s)),
              v.b*max(0,cos(r-s)) + v.r*max(0,cos(r+t-s)) + v.g*max(0,cos(r-t-s)),
              v.a);
}

void main(void) {
  float offs[3];
  offs[0] = -px;
  offs[1] = 0;
  offs[2] = +px;
  float f = 0;
  for (int x=0; x<3; ++x) for (int y=0; y<3; ++y)
    f += texture(d,vec2(varyingTexCoord.x+offs[x],1-varyingTexCoord.y+offs[y])).r;
//  dst.rgb = colour*texture(d,vec2(varyingTexCoord.x,1-varyingTexCoord.y)).r;
  dst = huerot(vec4(colour.r,colour.g,colour.b,1),
               cos(f+varyingTexCoord.x*9)/3 + sin(f-varyingTexCoord.y*9)/3 + f/4.5
        );
  //dst.rgb = texture(d,vec2(varyingTexCoord.x,1-varyingTexCoord.y)).rgb*vec3(1,10000000,10000000);
}
