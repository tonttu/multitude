#ifndef LUMINOUS_VERTEXARRAYGL_HPP
#define LUMINOUS_VERTEXARRAYGL_HPP

#include "OpenGL/ResourceHandleGL.hpp"

#include <QRegion>

namespace Luminous
{
  class VertexArrayGL : public ResourceHandleGL
  {
  public:
    VertexArrayGL(StateGL & state);
    ~VertexArrayGL();

    VertexArrayGL(VertexArrayGL && t);
    VertexArrayGL & operator=(VertexArrayGL && t);

    void bind();

    void upload(const VertexArray & vertexArray, ProgramGL * program);

    int generation() const { return m_generation; }

  private:
    void setVertexAttributes(const VertexArray & vertexArray);
    void setVertexDescription(const VertexDescription & description);

    int m_generation;
  };
}

#endif
