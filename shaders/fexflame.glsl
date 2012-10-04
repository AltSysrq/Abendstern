#version 120
/* Explosion frag shader that tries to produce a flame-like effect. */

uniform vec4 colour;
uniform float elapsedTime;
uniform int id;

varying /* in */ vec2 expCoord;

#define dst gl_FragColor

void main(void) {
  float r=expCoord.x*expCoord.x + expCoord.y*expCoord.y;
  if (r>1) discard;
  r=sqrt(r);
  float a=atan(expCoord.y,expCoord.x);
  float aphase1=sin((a-r)*9+elapsedTime*2.5+id);
  float aphase2=sin((a+r)*7-elapsedTime*1.5);
  float rphase1=sin((1/r)*2+elapsedTime);
  float rphase2=sin((1/r)*3-elapsedTime);
  float phase=abs(aphase1*aphase2*rphase1*rphase2);
  float cred=abs(sin(expCoord.x*13+expCoord.y*11));

  dst = colour;
  dst.g *= (0.5 + phase*cred*0.5);
  dst.b *= (0.4 + phase*rphase1*cred*0.6);
  dst.a *= phase*(1-r);
}
