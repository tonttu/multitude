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
    LUMINOUS_API VertexArrayGL(StateGL & state);
    LUMINOUS_API ~VertexArrayGL();

    LUMINOUS_API VertexArrayGL(VertexArrayGL && t);
    LUMINOUS_API VertexArrayGL & operator=(VertexArrayGL && t);

    LUMINOUS_API void bind();

    LUMINOUS_API void upload(const VertexArray & vertexArray, ProgramGL * program);

    LUMINOUS_API int generation() const { return m_generation; }

  private:
    void setVertexAttributes(const VertexArray & vertexArray, ProgramGL * program);
    void setVertexDescription(const VertexDescription & description, ProgramGL * program);

    int m_generation;

    std::set<std::shared_ptr<BufferGL> > m_associatedBuffers;
  };
}

#endif
