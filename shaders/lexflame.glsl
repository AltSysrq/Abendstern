/* Explosion frag shader that tries to produce a flame-like effect. */
#version 130

uniform vec4 colour;
uniform float elapsedTime;
uniform int id;

smooth in vec2 expCoord;

out vec4 dst;

float sq(float f) { return f*f; }
vec4 lumrot(vec4 v, float off) {
  float lum = sqrt(v.r*v.r +v.g*v.g + v.b*v.b);
  float th = (lum+off)*2*3.14159;
    return vec4(sq(cos(th)), sq(cos(th+3.14159/3)), sq(cos(th+2*3.14159/3)), v.a);
}

void main(void) {
  float r=expCoord.x*expCoord.x + expCoord.y*expCoord.y;
  if (r>1) discard;
  r=sqrt(r);
  float a=atan(expCoord.y,expCoord.x);
  dst = lumrot(colour, r)*lumrot(colour, a*elapsedTime*3);
  dst.a *= abs(sin(r*7)*sin(a*6));
}
