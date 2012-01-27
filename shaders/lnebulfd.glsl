/* Frag shader for nebula foreground */
#version 150

uniform vec3 colour;

const float maxViewDist = 8;

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
  vec2 p = varyingTexCoord*2-vec2(1,1);
  float dist = sqrt(p.x*p.x + p.y*p.y)/maxViewDist;
  dst.a = min(1.0, dist);
  dst.rgb = huerot(vec4(colour.r,colour.g,colour.b,1), dist*2).rgb;
}
