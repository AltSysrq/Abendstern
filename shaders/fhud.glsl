#version 120
/* Fragment shader for drawing the HUD reticle.
 * This is compatible with input from vbastex.
 */

uniform float topBarVal, botBarVal, leftBarVal, rightBarVal;
uniform vec3  topBarCol, botBarCol, leftBarCol, rightBarCol;

/* varying */ in vec2 varyingTexCoord;
/* varying */ in vec2 screenCoord;
varying out vec4 dst;

const float pi = 3.1415926;

void main(void) {
  //Convert tex coords to [-1,-1]x[1,1]
  vec2 v = varyingTexCoord*2 - vec2(1,1);
  float distSq = v.x*v.x + v.y*v.y;
  if (distSq > 1 || distSq < 0.85*0.85) discard;

  float angle = atan(v.y,v.x); //Angle [-pi,pi]
  if (angle > -3*pi/4 && angle < -1*pi/4) {
    float percent = (angle + 3*pi/4)*2/pi;
    if (percent > topBarVal) discard;
    else dst.rgb = topBarCol;
  } else if (angle > -1*pi/4 && angle < +1*pi/4) {
    float percent = 1 - (angle + 1*pi/4)*2/pi;
    if (percent > rightBarVal) discard;
    else dst.rgb = rightBarCol;
  } else if (angle > +1*pi/4 && angle < +3*pi/4) {
    float percent = 1 - (angle - 1*pi/4)*2/pi;
    if (percent > botBarVal) discard;
    else dst.rgb = botBarCol;
  } else {
    //Change from fragmented to [3/4*pi,5/4*pi]
    if (angle < 0) angle += 2*pi;
    float percent = (angle - 3*pi/4)*2/pi;
    if (percent > leftBarVal) discard;
    else dst.rgb = leftBarCol;
  }
  dst.a=1;
}
