#include "VertexArrayGL.hpp"
#include "Luminous/VertexArray.hpp"
#include "BufferGL.hpp"
#include "Luminous/RenderManager.hpp"
#include "RenderDriverGL.hpp"
#include "ProgramGL.hpp"

#include <QVector>

namespace Luminous
{
  VertexArrayGL::VertexArrayGL(StateGL & state)
    : ResourceHandleGL(state)
    , m_generation(0)
  {
    glGenVertexArrays(1, &m_handle);
  }

  VertexArrayGL::~VertexArrayGL()
  {
    if(m_handle)
      glDeleteVertexArrays(1, &m_handle);
  }

  VertexArrayGL::VertexArrayGL(VertexArrayGL && t)
    : ResourceHandleGL(std::move(t))
    , m_generation(t.m_generation)
  {
  }

  VertexArrayGL & VertexArrayGL::operator=(VertexArrayGL && t)
  {
    ResourceHandleGL::operator=(std::move(t));
    m_generation = t.m_generation;
    return *this;
  }

  void VertexArrayGL::bind()
  {
    if(m_state.setVertexArray(m_handle)) {
      glBindVertexArray(m_handle);
      GLERROR("VertexArrayGL::bind # glBindVertexArray");
    }

    touch();
  }

  void VertexArrayGL::upload(const VertexArray & vertexArray, ProgramGL * program)
  {
    m_generation = vertexArray.generation();

    // Bind and setup all buffers/attributes
    bind();

    if(program)
      program->bind();

    // Clear the associated buffers. Nothing will be released, as the driver
    // owns a copy of them, too.
    m_associatedBuffers.clear();

    setVertexAttributes(vertexArray, program);

    Buffer * index = RenderManager::getResource<Buffer>(vertexArray.indexBuffer());
    if (index != nullptr) {

      auto & bufferGL = m_state.driver().handle(*index);
      bufferGL.bind(Buffer::Index);
      /// Upload new data if we need to
      bufferGL.upload(*index, Buffer::Index);

      m_associatedBuffers.insert(m_state.driver().bufferPtr(*index));
    }
  }

  void VertexArrayGL::setVertexAttributes(const VertexArray & vertexArray, ProgramGL * program)
  {
    // Bind all vertex buffers
    for (size_t i = 0; i < vertexArray.bindingCount(); ++i) {
      VertexArray::Binding b = vertexArray.binding(i);
      // Attach buffer
      auto * buffer = RenderManager::getResource<Buffer>(b.buffer);
      assert(buffer != nullptr);

      auto & bufferGL = m_state.driver().handle(*buffer);
      bufferGL.bind(Buffer::Vertex);
      /// Upload new data if we need to
      bufferGL.upload(*buffer, Buffer::Vertex);

      setVertexDescription(b.description, program);

      m_associatedBuffers.insert(m_state.driver().bufferPtr(*buffer));
    }
  }

  void VertexArrayGL::setVertexDescription(const VertexDescription &description, ProgramGL * program)
  {
    // Set buffer attributes from its bound VertexDescription
    for (size_t attrIndex = 0; attrIndex < description.attributeCount(); ++attrIndex) {

      VertexAttribute attr = description.attribute(attrIndex);

      GLint location = program->attributeLocation(attr.name);
      if (location == -1) {
        Radiant::warning("Unable to bind vertex attribute %s", attr.name.data());
      } else {

        GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);

        glVertexAttribPointer(location, attr.count, attr.type, normalized, description.vertexSize(), reinterpret_cast<GLvoid *>(attr.offset));
        GLERROR("VertexArrayGL::setVertexDescription # glVertexAttribPointer");

        glEnableVertexAttribArray(location);
        GLERROR("VertexArrayGL::setVertexDescription # glEnableVertexAttribArray");
      }
    }
  }
}
