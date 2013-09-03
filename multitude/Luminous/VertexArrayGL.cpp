/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

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
    assert(vertexArray.indexBuffer() == 0 || index != nullptr);

    if (index != nullptr) {
      auto & bufferGL = m_state.driver().handle(*index);
      bufferGL.bind(Buffer::INDEX);
      /// Upload new data if we need to
      bufferGL.upload(*index, Buffer::INDEX);
      m_associatedBuffers.insert(m_state.driver().bufferPtr(*index));
    }
  }

  void VertexArrayGL::setVertexAttributes(const VertexArray & vertexArray, ProgramGL * program)
  {
    assert(m_generation == vertexArray.generation());
    // Bind all vertex buffers
    for (size_t i = 0; i < vertexArray.bindingCount(); ++i) {
      VertexArray::Binding b = vertexArray.binding(i);
      // Attach buffer
      auto & bufferGL = m_state.driver().handle(*b.buffer);
      /// Upload new data if we need to
      bufferGL.upload(*b.buffer, Buffer::VERTEX);

      // Set vertex attributes
      bufferGL.bind(Buffer::VERTEX);
      setVertexDescription(b.description, program);

      m_associatedBuffers.insert(m_state.driver().bufferPtr(*b.buffer));
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
