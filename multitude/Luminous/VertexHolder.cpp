#include "VertexHolder.hpp"

#include "RenderContext.hpp"
#include "Utils.hpp"

namespace Luminous
{

  VertexHolder::VertexHolder()
  {
    strcpy(m_vertextype, "Undefined");
  }

  VertexHolder::~VertexHolder()
  {}

  RenderPacket::RenderPacket()
    : m_program(0),
      m_func(RectVertex::render)
  {}

  RenderPacket::~RenderPacket()
  {}

  void RectVertex::render(RenderContext &r, RenderPacket & rp)
  {
    if(! & rp)
      return;

    if(rp.empty())
      return;

    Utils::glCheck("RectVertex::render # 1");

    assert(rp.program() != 0);

    const int vsize = sizeof(RectVertex);

    RectVertex & vr = * rp.vertexData<RectVertex>();

    GLSLProgramObject & prog = * rp.program();

    prog.setUniformMatrix4("view_transform", r.viewTransform());

    Utils::glCheck("RectVertex::render # 2");

    rp.vbo().bind();
    rp.vbo().fill(rp.vertexData<RectVertex>(), rp.vertices().bytes(), Luminous::VertexBuffer::DYNAMIC_DRAW);

    const char * func = "RectVertex::render";

    VertexAttribArrayStep ls(prog, "location", 2, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_location, vr), func);
    VertexAttribArrayStep cs(prog, "color", 4, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_color, vr), func);
    VertexAttribArrayStep ts(prog, "tex_coord", 2, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_texCoord, vr), func);
    VertexAttribArrayStep ut(prog, "use_tex", 1, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_useTexture, vr), func);

    VertexAttribArrayStep mr1(prog, "object_transform_r1", 3, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objectTransform[0], vr), func);
    VertexAttribArrayStep mr2(prog, "object_transform_r2", 3, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objectTransform[1], vr), func);
    VertexAttribArrayStep mr3(prog, "object_transform_r3", 3, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objectTransform[2], vr), func);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(rp.vertices().count<RectVertex>()));

    rp.clear();
    rp.vbo().unbind(); // Should not really need call this
    Utils::glCheck("RectVertex::render # 3");
  }

  void CircleVertex::render(RenderContext &r, RenderPacket & rp)
  {
    if(! & rp)
      return;

    if(rp.empty())
      return;

    Utils::glCheck("CircleVertex::render # 1");

    assert(rp.program() != 0);

    const int vsize = sizeof(CircleVertex);

    CircleVertex & vr = * rp.vertexData<CircleVertex>();

    GLSLProgramObject & prog = * rp.program();

    prog.setUniformMatrix4("view_transform", r.viewTransform());

    Utils::glCheck("CircleVertex::render # 2");

    rp.vbo().bind();
    rp.vbo().fill(rp.vertexData<CircleVertex>(), rp.vertices().bytes(), Luminous::VertexBuffer::DYNAMIC_DRAW);

    const char * func = "RectVertex::render";

    VertexAttribArrayStep ls(prog, "location", 2, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_location, vr));
    VertexAttribArrayStep cs(prog, "color", 4, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_color, vr));
    VertexAttribArrayStep ts(prog, "tex_coord", 2, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_texCoord, vr));
    VertexAttribArrayStep os(prog, "obj_coord", 2, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objCoord, vr));
    VertexAttribArrayStep ut(prog, "use_tex", 1, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_useTexture, vr));

    VertexAttribArrayStep mr1(prog, "object_transform_r1", 3, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objectTransform[0], vr), func);
    VertexAttribArrayStep mr2(prog, "object_transform_r2", 3, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objectTransform[1], vr), func);
    VertexAttribArrayStep mr3(prog, "object_transform_r3", 3, GL_FLOAT, GL_FALSE, vsize, offsetBytes(vr.m_objectTransform[2], vr), func);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(rp.vertices().count<RectVertex>()));

    rp.clear();
    rp.vbo().unbind(); // Should not really call this
    Utils::glCheck("CircleVertex::render # 3");
  }


}
