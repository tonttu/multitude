#version 120
#extension GL_EXT_geometry_shader4 : enable

varying in float size[];
varying in vec2 vsiz[];

void main(void)
{
  for(int i = 0; i < gl_VerticesIn; i++) {

    vec2 s = vec2(20.0 / vsiz[i].x, 20.0 / vsiz[i].y);
    /* vec2 s = vec2(0.01, 0.01); */

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
