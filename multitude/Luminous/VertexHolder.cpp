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

    Utils::glCheck("RenderPacket::render # 1");

    assert(rp.program() != 0);

    const int vsize = sizeof(RectVertex);

    RectVertex & vr = * rp.vertexData<RectVertex>();

    GLSLProgramObject & prog = * rp.program();

    prog.setUniformMatrix4("view_transform", r.viewTransform());

    int aloc = prog.getAttribLoc("location");
    int acol = prog.getAttribLoc("color");
    int atex = prog.getAttribLoc("tex_coord");
    int aute = prog.getAttribLoc("use_tex");

    int omr1 = prog.getAttribLoc("object_transform_r1");
    int omr2 = prog.getAttribLoc("object_transform_r2");
    int omr3 = prog.getAttribLoc("object_transform_r3");

//    if((aloc < 0) || (acol < 0) || (atex < 0) || (aute < 0)) {
//      fatal("RenderContext::flush # %d vertices %p %p %d", (int) rp.vertices().count<RectVertex>(),
//            m_data->m_program, &*m_data->m_basic_shader, (int) prog.getAttribLoc("location"));
//    }
    Utils::glCheck("RenderPacket::render # 2");

    /*
    Radiant::info("shader attribs are %d %d %d %d : %d %d %d for %d", aloc, acol, atex, aute,
                  omr1, omr2, omr3,
                  (int) rp.vertices().count<RectVertex>());
                  */
    rp.vbo().bind();
    rp.vbo().fill(rp.vertexData<RectVertex>(), rp.vertices().bytes(), Luminous::VertexBuffer::DYNAMIC_DRAW);

    VertexAttribArrayStep ls(aloc, 2, GL_FLOAT, vsize, offsetBytes(vr.m_location, vr));
    VertexAttribArrayStep cs(acol, 4, GL_FLOAT, vsize, offsetBytes(vr.m_color, vr));
    VertexAttribArrayStep ts(atex, 2, GL_FLOAT, vsize, offsetBytes(vr.m_texCoord, vr));
    VertexAttribArrayStep ut(aute, 1, GL_FLOAT, vsize, offsetBytes(vr.m_useTexture, vr));

    VertexAttribArrayStep mr1(omr1, 3, GL_FLOAT, vsize, offsetBytes(vr.m_objectTransform[0], vr));
    VertexAttribArrayStep mr2(omr2, 3, GL_FLOAT, vsize, offsetBytes(vr.m_objectTransform[1], vr));
    VertexAttribArrayStep mr3(omr3, 3, GL_FLOAT, vsize, offsetBytes(vr.m_objectTransform[2], vr));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, GLsizei(rp.vertices().count<RectVertex>()));

    rp.clear();
    rp.vbo().unbind(); // Should not really call this
    Utils::glCheck("RenderPacket::render # 3");
  }


}
