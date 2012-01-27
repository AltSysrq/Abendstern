/* Basic texture fragment shader. Multiplies a uniform colour
 * by the alpha component of the texture.
 */
#version 130

smooth in vec2 varyingTexCoord;
uniform sampler2D colourMap;
uniform vec4 modColour;
out vec4 dst;

float sq(float f) { return f*f; }
vec4 lumrot(vec4 v, float off) {
  float lum = sqrt(v.r*v.r +v.g*v.g + v.b*v.b);
  float th = (lum+off)*2*3.14159;
    return vec4(sq(cos(th)), sq(cos(th+3.14159/3)), sq(cos(th+2*3.14159/3)), v.a);
}

void main(void) {
  float coloura = texture(colourMap, varyingTexCoord).a;
  if (coloura < 0.01) discard;
  vec4 colour = lumrot(modColour,
          cos(varyingTexCoord.x*varyingTexCoord.x + varyingTexCoord.y*varyingTexCoord.y));
  colour.a *= coloura;
  dst.rgb = colour.rgb;
  dst.a = colour.a;
}
