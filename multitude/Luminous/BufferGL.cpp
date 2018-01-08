/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

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
    , m_generation(0)
  {    
    m_state.opengl().glGenBuffers(1, &m_handle);
  }

  BufferGL::BufferGL(StateGL & state, Buffer::Usage usage)
    : ResourceHandleGL(state)
    , m_usage(usage)
    , m_size(0)
    , m_allocatedSize(0)
    , m_uploaded(0)
    , m_generation(0)
  {
    m_state.opengl().glGenBuffers(1, &m_handle);
  }

  BufferGL::~BufferGL()
  {
    if(m_handle)
      m_state.opengl().glDeleteBuffers(1, &m_handle);
  }

  BufferGL::BufferGL(BufferGL && t)
    : ResourceHandleGL(std::move(t))
    , m_usage(t.m_usage)
    , m_size(t.m_size)
    , m_allocatedSize(t.m_allocatedSize)
    , m_uploaded(t.m_size)
    , m_generation(t.m_generation)
  {
  }

  BufferGL & BufferGL::operator=(BufferGL && t)
  {
    ResourceHandleGL::operator=(std::move(t));
    return *this;
  }

  void BufferGL::bind(Buffer::Type type)
  {
    m_state.opengl().glBindBuffer(type, m_handle);
    GLERROR("BufferGL::bind # glBindBuffer");

    touch();
  }

  void BufferGL::upload(const Buffer & buffer, Buffer::Type type)
  {
    // Reset usage timer
    touch();

    // Update if dirty
    if(m_generation < buffer.generation()) {
      bind(type);

      /// @todo incremental upload
      if(buffer.size() != m_allocatedSize || buffer.usage() != m_usage) {
        m_state.opengl().glBufferData(type, buffer.size(), buffer.data(), buffer.usage());
        GLERROR("BufferGL::upload # glBufferData");
      } else if (buffer.data()) {
        m_state.opengl().glBufferSubData(type, 0, buffer.size(), buffer.data());
        GLERROR("BufferGL::upload # glBufferSubData");
      }

      m_generation = buffer.generation();
      m_size = buffer.size();
      m_allocatedSize = m_size;
      m_uploaded = buffer.size();
      m_usage = buffer.usage();
    }
  }

  void BufferGL::upload(Buffer::Type type, int offset, std::size_t length, const void * data)
  {
    touch();
    bind(type);
    if (length + offset > m_size)
      m_size = length + offset;
    if (m_allocatedSize < m_size)
      allocate(type);
    m_state.opengl().glBufferSubData(type, offset, length, data);
    GLERROR("BufferGL::upload # glBufferSubData");
  }

  void * BufferGL::map(Buffer::Type type, int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access)
  {
    touch();

    BufferMapping & mappings = m_state.bufferMaps()[m_handle];

    if(mappings.data) {
      if(mappings.access == access.asInt() && mappings.offset == offset && mappings.length == length) {
        assert(mappings.data);
        return mappings.data;
      }

      bind(type);
      m_state.opengl().glUnmapBuffer(type);
      GLERROR("BufferGL::map # glUnmapBuffer");
    } else {
      bind(type);
    }

    if (length + offset > m_size)
      m_size = length + offset;
    if(m_allocatedSize < m_size)
      allocate(type);

    mappings.access = access.asInt();
    mappings.target = type;
    mappings.offset = offset;
    mappings.length = length;

    if(offset + length > m_size) {
      Radiant::warning("BufferGL::map # Attempting to map too large buffer range (%d [offset] + %d [length] > %d [m_size])",
                       offset, int(length), int(m_size));
    }

    mappings.data = m_state.opengl().glMapBufferRange(mappings.target, mappings.offset, mappings.length, mappings.access);
    GLERROR("BufferGL::map # glMapBufferRange");
    assert(mappings.data);

    return mappings.data;
  }

  void BufferGL::unmap(Buffer::Type type, int offset, std::size_t length)
  {
    touch();

    auto it = m_state.bufferMaps().find(m_handle);
    if(it == m_state.bufferMaps().end()) {
      Radiant::warning("BufferGL::unmap # buffer not mapped");
      return;
    }

    bind(type);

    if(length != std::size_t(-1) && (it->second.access & GL_MAP_FLUSH_EXPLICIT_BIT)) {
      m_state.opengl().glFlushMappedBufferRange(type, offset, length);
      GLERROR("BufferGL::unmap # glFlushMappedBufferRange");
    }

    m_state.opengl().glUnmapBuffer(type);
    GLERROR("BufferGL::unmap # glUnmapBuffer");

    m_state.bufferMaps().erase(it);
  }

  void BufferGL::allocate(Buffer::Type type)
  {
    touch();

    m_state.opengl().glBufferData(type, m_size, nullptr, m_usage);
    GLERROR("BufferGL::allocate # glBufferData");
    m_allocatedSize = m_size;
  }
}
