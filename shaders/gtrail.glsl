/* Geom shader that transforms a line strip into a
 * triangle strip. It is intended to be used with
 * GL_LINE_STRIP_ADJACENCY.
 */

#version 150

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices=6) out;

uniform mat4 transform;

flat in vec4 vertexColour[];
flat in float vertexWidth[];
smooth out vec4 varyingColour;
smooth out float dist;

void main(void) {
  vec2 v1 = gl_in[1].gl_Position.xy;
  vec2 v2 = gl_in[2].gl_Position.xy;
  vec2 v3 = gl_in[3].gl_Position.xy;

  if (vertexColour[1].a <= 0 && vertexColour[2].a <= 0) return;

  vec2 v12 = normalize(v2-v1);
  vec2 v23 = normalize(v3-v2);
  vec2 angle1 = vec2(-v12.y, v12.x);
  vec2 angle2 = vec2(-v23.y, v23.x);

  vec2 fv;

  fv = v1 - angle1*vertexWidth[1];
  dist = -1;
  varyingColour = vertexColour[1];
  gl_Position = vec4(fv.x, fv.y, 0, 1)*transform;
  EmitVertex();

  fv = v1 + angle1*vertexWidth[1];
  dist = +1;
  varyingColour = vertexColour[1];
  gl_Position = vec4(fv.x, fv.y, 0, 1)*transform;
  EmitVertex();

  fv = v2 - angle2*vertexWidth[2];
  dist = -1;
  varyingColour = vertexColour[2];
  gl_Position = vec4(fv.x, fv.y, 0, 1)*transform;
  EmitVertex();
  EndPrimitive();


  fv = v1 + angle1*vertexWidth[1];
  dist = +1;
  varyingColour = vertexColour[1];
  gl_Position = vec4(fv.x, fv.y, 0, 1)*transform;
  EmitVertex();

  fv = v2 - angle2*vertexWidth[2];
  dist = -1;
  varyingColour = vertexColour[2];
  gl_Position = vec4(fv.x, fv.y, 0, 1)*transform;
  EmitVertex();

  fv = v2 + angle2*vertexWidth[2];
  dist = +1;
  varyingColour = vertexColour[2];
  gl_Position = vec4(fv.x, fv.y, 0, 1)*transform;
  EmitVertex();
  EndPrimitive();
}
