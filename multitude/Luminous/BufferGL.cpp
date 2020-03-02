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
    , m_size(buffer.bufferSize())
    , m_allocatedSize(0)
    , m_generation(0)
  {    
    m_state.opengl().glGenBuffers(1, &m_handle);
  }

  BufferGL::BufferGL(StateGL & state, Buffer::Usage usage)
    : ResourceHandleGL(state)
    , m_usage(usage)
    , m_size(0)
    , m_allocatedSize(0)
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

  void BufferGL::unbind(Buffer::Type type)
  {
    m_state.opengl().glBindBuffer(type, 0);
  }

  void BufferGL::upload(const Buffer & buffer, Buffer::Type type)
  {
    // Reset usage timer
    touch();

    const Buffer::DirtyRegion dirtyRegion = buffer.takeDirtyRegion(m_state.threadIndex());
    bool recreate = m_generation < buffer.generation();

    if (!recreate) {
      if (dirtyRegion.dataEnd == dirtyRegion.dataBegin)
        return;

      /// Do partial upload
      bind(type);
      m_state.opengl().glBufferSubData(type, dirtyRegion.dataBegin,
                                       dirtyRegion.dataEnd - dirtyRegion.dataBegin,
                                       static_cast<const uint8_t*>(buffer.data()) + dirtyRegion.dataBegin);
      GLERROR("BufferGL::upload # glBufferSubData");
      return;
    }

    if (recreate) {
      bind(type);

      if(buffer.bufferSize() != m_allocatedSize || buffer.usage() != m_usage) {
        if (buffer.bufferSize() == buffer.dataSize()) {
          m_state.opengl().glBufferData(type, buffer.bufferSize(), buffer.data(), buffer.usage());
          GLERROR("BufferGL::upload # glBufferData");
        } else {
          m_state.opengl().glBufferData(type, buffer.bufferSize(), nullptr, buffer.usage());
          GLERROR("BufferGL::upload # glBufferData");

          m_state.opengl().glBufferSubData(type, 0, buffer.dataSize(), buffer.data());
          GLERROR("BufferGL::upload # glBufferSubData");
        }
      } else if (buffer.data()) {
        m_state.opengl().glBufferSubData(type, 0, buffer.dataSize(), buffer.data());
        GLERROR("BufferGL::upload # glBufferSubData");
      }

      m_generation = buffer.generation();
      m_size = buffer.bufferSize();
      m_allocatedSize = m_size;
      m_usage = buffer.usage();
    }
  }

  void BufferGL::upload(Buffer::Type type, int offset, std::size_t length, const void * data)
  {
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
    bind(type);

    if (length + offset > m_size)
      m_size = length + offset;
    if(m_allocatedSize < m_size)
      allocate(type);

    m_mappedAccess = access;

    void * data = m_state.opengl().glMapBufferRange(type, offset, length, access.asInt());
    GLERROR("BufferGL::map # glMapBufferRange");
    assert(data);

    return data;
  }

  void BufferGL::unmap(Buffer::Type type, int offset, std::size_t length)
  {
    bind(type);

    if(length != std::size_t(-1) && (m_mappedAccess & Buffer::MAP_FLUSH_EXPLICIT)) {
      m_state.opengl().glFlushMappedBufferRange(type, offset, length);
      GLERROR("BufferGL::unmap # glFlushMappedBufferRange");
    }

    m_state.opengl().glUnmapBuffer(type);
    GLERROR("BufferGL::unmap # glUnmapBuffer");

    m_mappedAccess.clear();
  }

  void BufferGL::allocate(Buffer::Type type)
  {
    touch();

    m_state.opengl().glBufferData(type, m_size, nullptr, m_usage);
    GLERROR("BufferGL::allocate # glBufferData");
    m_allocatedSize = m_size;
  }
}
