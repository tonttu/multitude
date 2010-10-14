#version 120
#extension GL_EXT_geometry_shader4 : enable

varying in float gs_size[];
varying in float gs_alpha[];
uniform vec2 vsiz;

varying out float fs_alpha;

void main(void)
{
  for(int i = 0; i < gl_VerticesIn; i++) {

    /* Since we are working in normalized coordinates, we need to account
       for the viewport size. */
    vec2 s = vec2(gs_size[i] / vsiz.x, gs_size[i] / vsiz.y);

    fs_alpha = gs_alpha[i];
    vec2 p = gl_PositionIn[i].xy + vec2(-s.x, -s.y);
    gl_Position = vec4(p, 0, 1);
    EmitVertex();

    p = gl_PositionIn[i].xy + vec2(s.x, -s.y);
    gl_Position = vec4(p, 0, 1);
    EmitVertex();

    p = gl_PositionIn[i].xy + vec2(-s.x, s.y);
    gl_Position = vec4(p, 0, 1);
    EmitVertex();

    EndPrimitive();

    p = gl_PositionIn[i].xy + vec2(s.x, -s.y);
    gl_Position = vec4(p, 0, 1);
    EmitVertex();

    p = gl_PositionIn[i].xy + vec2(-s.x, s.y);
    gl_Position = vec4(p, 0, 1);
    EmitVertex();

    p = gl_PositionIn[i].xy + vec2(s.x, s.y);
    gl_Position = vec4(p, 0, 1);
    EmitVertex();

    EndPrimitive();
  }
}
