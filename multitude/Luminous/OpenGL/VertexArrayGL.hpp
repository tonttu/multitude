#ifndef LUMINOUS_VERTEXARRAYGL_HPP
#define LUMINOUS_VERTEXARRAYGL_HPP

#include "ResourceHandleGL.hpp"

#include <QRegion>

#include <set>
#include <memory>

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
    void setVertexAttributes(const VertexArray & vertexArray, ProgramGL * program);
    void setVertexDescription(const VertexDescription & description, ProgramGL * program);

    int m_generation;

    std::set<std::shared_ptr<BufferGL> > m_associatedBuffers;
  };
}

#endif
