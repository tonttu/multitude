#include "BufferGL.hpp"

#include <QVector>
#include <cassert>

namespace Luminous
{
  BufferGL::BufferGL(StateGL & state, const Buffer & buffer)
    : ResourceHandleGL(state)
    , m_usage(buffer.usage())
    , m_size(buffer.size())
    , m_allocatedSize(0)
    , m_uploaded(0)
    , m_target(buffer.type())
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
    , m_allocatedSize(t.m_allocatedSize)
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
    GLERROR("BufferGL::bind # glBindBuffer");
  }

  void BufferGL::upload(Buffer & buffer)
  {
    // Reset usage timer
    touch();

    // Update if dirty
    if(m_generation < buffer.generation()) {
      m_target = buffer.type();
      bind();

      /// @todo incremental upload
      if(buffer.size() != m_size || buffer.usage() != m_usage) {
        glBufferData(m_target, buffer.size(), buffer.data(), buffer.usage());
        GLERROR("BufferGL::upload # glBufferData");
      } else {
        glBufferSubData(m_target, 0, buffer.size(), buffer.data());
        GLERROR("BufferGL::upload # glBufferSubData");
      }

      m_generation = buffer.generation();
      m_size = buffer.size();
      m_allocatedSize = m_size;
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
      if(mappings.access == access.asInt() && mappings.offset == offset && mappings.length == length) {
        assert(mappings.data);
        return mappings.data;
      }

      bind();
      glUnmapBuffer(m_target);
      GLERROR("BufferGL::map # glUnmapBuffer");
    }

    bind();

    if(m_allocatedSize < m_size)
      allocate();

    mappings.access = access.asInt();
    mappings.target = m_target;
    mappings.offset = offset;
    mappings.length = length;

    if(offset + length > m_size) {
      Radiant::warning("BufferGL::map # Attempting to map too large buffer range (%d [offset] + %d [length] > %d [m_size])",
                       offset, int(length), int(m_size));
    }

    mappings.data = glMapBufferRange(mappings.target, mappings.offset, mappings.length, mappings.access);
    GLERROR("BufferGL::map # glMapBufferRange");
    assert(mappings.data);

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
    GLERROR("BufferGL::unmap # glUnmapBuffer");

    m_state.bufferMaps().erase(it);
  }

  void BufferGL::allocate()
  {
    glBufferData(m_target, m_size, nullptr, m_usage);
    GLERROR("BufferGL::allocate # glBufferData");
    m_allocatedSize = m_size;
  }
}
