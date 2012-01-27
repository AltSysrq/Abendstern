/* Fragment shader to perform the nebula-base-update
 * function. The simSideSz value MUST be the same as
 * defined in C++.
 *
 * Explanations for the equations used here can be
 * found in src/background/nebula.cxx.
 */

#version 150

const float simSideSz = 8*128, px = 1.0/simSideSz, screens=8;

/* The change in location of the bottom-left, in texture coordinates. */
uniform vec2 delta, botleft;
uniform sampler2D d, naturals;
uniform float time, mass, viscosity, fieldWidth, fieldHeight, prubber, vrubber;

smooth in vec2 pos;
out vec4 dst;

float p(vec2 v) { return texture(d,v/*vec2(clamp(v.x,0,1), clamp(v.y,0,1))*/).r; }
vec2 v(vec2 p) { return texture(d,p/*vec2(clamp(p.x,0,1), clamp(p.y,0,1))*/).gb; }

vec4 nat(void) {
  vec2 tc = pos*screens*vec2(1/fieldWidth, 1/fieldHeight)+botleft;
  return texture(naturals, vec2(clamp(tc.x,0,1), clamp(tc.y,0,1)));
}

float cl(vec2 v, float comp) {
  return abs(comp)*comp/sqrt(v.x*v.x+v.y*v.y);
}
float clx(vec2 v) { return cl(v,v.x); }
float cly(vec2 v) { return cl(v,v.y); }

float calcp(void) {
  vec2 V=v(pos);
  return p(pos) + time * (
    max(0.0, -p(pos+vec2(px,0))*clx(v(pos+vec2(px,0))))
  + max(0.0, +p(pos-vec2(px,0))*clx(v(pos-vec2(px,0))))
  + max(0.0, -p(pos+vec2(0,px))*cly(v(pos+vec2(0,px))))
  + max(0.0, +p(pos-vec2(0,px))*cly(v(pos-vec2(0,px))))
  - p(pos)*sqrt(V.x*V.x+V.y*V.y));
}

vec2 calcv(void) {
  return v(pos) + time/mass/max(0.001,p(pos)) * (
    vec2(p(pos-vec2(px,0)) - p(pos+vec2(px,0)),
         p(pos-vec2(0,px)) - p(pos+vec2(0,px)))
  + viscosity * (p(pos+vec2(px,0))*v(pos+vec2(px,0)) + p(pos-vec2(px,0))*v(pos-vec2(px,0))
                +p(pos+vec2(0,px))*v(pos+vec2(0,px)) + p(pos-vec2(0,px))*v(pos-vec2(0,px))
                -4*v(pos)*p(pos)));
}

void main(void) {
  /*
  if (pos.x < delta.x+px || pos.x >= 1+delta.x-px
  ||  pos.y < delta.y+px || pos.y >= 1+delta.y-px) {
    //On the edge, just copy natural position and calculate conventional velocity
    //vec2 newv = calcv();
    //dst = vec4(nat().r, clamp(newv.x, -0.005,+0.005), clamp(newv.y,-0.005,+0.005),1);
    dst=nat();
    dst.a=1;
    return;
  }*/
  vec2 newv = calcv();
  newv = vec2(clamp(newv.x, -0.05,+0.05), clamp(newv.y,-0.05,+0.05));
  float newp = clamp(calcp(), 0, 10);
  vec4 n = nat();
  newp += time*prubber*(n.r-newp);
  newv += time*vrubber*(n.gb-newv);
  dst = vec4(newp,newv.x,newv.y,1);
}
