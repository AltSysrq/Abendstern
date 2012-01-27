/* Basic texture fragment shader. Multiplies a uniform colour
 * by the texture samples, emulating GL_MODULATE.
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
  vec4 colour=texture(colourMap, varyingTexCoord);
  colour *= modColour;
  if (colour.a < 0.01) discard;
  dst = lumrot(colour,
               cos(varyingTexCoord.x*varyingTexCoord.x + varyingTexCoord.y*varyingTexCoord.y));

}
