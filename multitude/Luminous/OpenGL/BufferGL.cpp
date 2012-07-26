#include "BufferGL.hpp"

#include <QVector>
#include <cassert>

namespace Luminous
{
  BufferGL::BufferGL(StateGL & state)
    : ResourceHandleGL(state)
    , m_usage(Buffer::StaticDraw)
    , m_size(0)
    , m_uploaded(0)
    , m_target(0)
    , m_generation(0)
  {    
    glGenBuffers(1, &m_handle);
  }

  BufferGL::~BufferGL()
  {
    if(m_handle)
      glDeleteBuffers(1, &m_handle);
  }

  BufferGL::BufferGL(BufferGL && t)
    : ResourceHandleGL(std::move(t))
    , m_usage(t.m_usage)
    , m_size(t.m_size)
    , m_uploaded(t.m_size)
    , m_target(t.m_target)
    , m_generation(t.m_generation)
  {
  }

  BufferGL & BufferGL::operator=(BufferGL && t)
  {
    ResourceHandleGL::operator=(std::move(t));
    return *this;
  }

  void BufferGL::bind()
  {
    glBindBuffer(m_target, m_handle);
  }

  void BufferGL::upload(Buffer & buffer)
  {
    // Reset usage timer
    touch();

    // Update if dirty
    if(m_generation < buffer.generation()) {
      bind();

      /// @todo incremental upload
      if(buffer.size() != m_size || buffer.usage() != m_usage) {
        glBufferData(m_target, buffer.size(), buffer.data(), buffer.usage());
      } else {
        glBufferSubData(m_target, 0, buffer.size(), buffer.data());
      }

      m_generation = buffer.generation();
      m_size = buffer.size();
      m_uploaded = buffer.size();
      m_usage = buffer.usage();
    }
  }

  void * BufferGL::map(int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access)
  {
    assert(m_target == Buffer::Vertex ||
           m_target == Buffer::Index ||
           m_target == Buffer::Uniform);

    BufferMapping & mappings = m_state.bufferMaps()[m_handle];

    if(mappings.data) {
      if(mappings.access == access.asInt() && mappings.offset == offset && mappings.length == length)
        return mappings.data;

      bind();
      glUnmapBuffer(m_target);
    }

    bind();

    mappings.access = access.asInt();
    mappings.target = m_target;
    mappings.offset = offset;
    mappings.length = length;
    mappings.data = glMapBufferRange(mappings.target, mappings.offset, mappings.length, mappings.access);

    return mappings.data;
  }

  void BufferGL::unmap()
  {
    auto it = m_state.bufferMaps().find(m_handle);
    if(it == m_state.bufferMaps().end()) {
      Radiant::warning("BufferGL::unmap # buffer not mapped");
      return;
    }

    bind();

    glUnmapBuffer(m_target);

    m_state.bufferMaps().erase(it);
  }

}
