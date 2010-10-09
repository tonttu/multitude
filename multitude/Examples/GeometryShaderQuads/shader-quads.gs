#version 120
#extension GL_EXT_geometry_shader4 : enable

varying in float gs_size[];
varying in float gs_alpha[];
uniform vec2 vsiz;

varying out float fs_alpha;

void main(void)
{
  for(int i = 0; i < gl_VerticesIn; i++) {

    vec2 s = vec2(gs_size[i] / vsiz.x, gs_size[i] / vsiz.y);
    /* vec2 s = vec2(0.01, 0.01); */

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
