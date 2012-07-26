#include "VertexArrayGL.hpp"
#include "VertexArray.hpp"
#include "BufferGL.hpp"
#include "RenderManager.hpp"
#include "RenderDriverGL.hpp"

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
    if(m_state.setVertexArray(m_handle))
      glBindVertexArray(m_handle);
  }

  void VertexArrayGL::upload(const VertexArray &vertexArray)
  {
    m_generation = vertexArray.generation();

    // Bind and setup all buffers/attributes
    glBindVertexArray(m_handle);

    //if(programHandle) bindShaderProgram(*programHandle);

    setVertexAttributes(vertexArray);

    const Buffer * index = RenderManager::getResource<Buffer>(vertexArray.indexBuffer());
    if (index != nullptr) {

      assert(index->type() == GL_ELEMENT_ARRAY_BUFFER);
      auto & bufferGL = m_state.driver().handle(*index);
      bufferGL.bind();
    }
  }

  void VertexArrayGL::setVertexAttributes(const VertexArray & vertexArray)
  {
    // Bind all vertex buffers
    for (size_t i = 0; i < vertexArray.bindingCount(); ++i) {
      VertexArray::Binding b = vertexArray.binding(i);
      // Attach buffer
      auto * buffer = RenderManager::getResource<Buffer>(b.buffer);
      assert(buffer != nullptr);

      assert(buffer->type() == GL_ARRAY_BUFFER);
      auto & bufferGL = m_state.driver().handle(*buffer);
      bufferGL.bind();

      setVertexDescription(b.description);
    }
  }


  void VertexArrayGL::setVertexDescription(const VertexDescription &description)
  {
    // Set buffer attributes from its bound VertexDescription
    for (size_t attrIndex = 0; attrIndex < description.attributeCount(); ++attrIndex) {

      VertexAttribute attr = description.attribute(attrIndex);

      GLint location = glGetAttribLocation(m_state.program(), attr.name.toAscii().data());

      if (location == -1) {
        Radiant::warning("Unable to bind vertex attribute %s", attr.name.toAscii().data());
      } else {

        GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);

        glVertexAttribPointer(location, attr.count, attr.type, normalized, description.vertexSize(), reinterpret_cast<GLvoid *>(attr.offset));

        glEnableVertexAttribArray(location);
      }
    }
  }
}
