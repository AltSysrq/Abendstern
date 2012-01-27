/* Simple identity frag shader, LSD version. */
#version 130

smooth in vec4 varyingColour;
out vec4 dst;

smooth in vec4 position;
flat in float transsum;

vec4 huerot(vec4 v, float percent) {
  float r=percent*3.14159*2;
  float t=3.14159/3*2;
  return vec4(v.r*max(0,cos(r)) + v.g*max(0,cos(r+t)) + v.b*max(0,cos(r-t)),
              v.g*max(0,cos(r)) + v.b*max(0,cos(r+t)) + v.r*max(0,cos(r-t)),
              v.b*max(0,cos(r)) + v.r*max(0,cos(r+t)) + v.g*max(0,cos(r-t)),
              v.a);
}

void main(void) {
  dst=huerot(varyingColour,
             position.x+cos(5*position.y)+0.1*sin(10*position.x*position.y*transsum))/2
     +huerot(vec4(sqrt(varyingColour.r*varyingColour.r
                      +varyingColour.g*varyingColour.g
                      +varyingColour.b*varyingColour.b),0,0,varyingColour.a),
             position.y+sin(6*position.x)+0.1*cos( 7*position.x*position.y*transsum))/2;
}
