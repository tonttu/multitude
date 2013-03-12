/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_BUFFERGL_HPP
#define LUMINOUS_BUFFERGL_HPP

#include "ResourceHandleGL.hpp"
#include "Luminous/Buffer.hpp"

namespace Luminous
{
  class BufferGL : public ResourceHandleGL
  {
  public:
    LUMINOUS_API BufferGL(StateGL & state, const Buffer & buffer);
    LUMINOUS_API ~BufferGL();

    LUMINOUS_API BufferGL(BufferGL && t);
    LUMINOUS_API BufferGL & operator=(BufferGL && t);

    LUMINOUS_API void bind(Buffer::Type type);
    LUMINOUS_API void upload(const Buffer &buffer, Buffer::Type type);
    LUMINOUS_API void upload(Buffer::Type type, int offset, std::size_t length, const void * data);

    LUMINOUS_API void * map(Buffer::Type type, int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access);
    LUMINOUS_API void unmap(Buffer::Type type, int offset = 0, std::size_t length = std::size_t(-1));

  private:
    void allocate(Buffer::Type type);

  private:
    Buffer::Usage m_usage;
    size_t m_size;        // Size (in bytes)
    size_t m_allocatedSize;
    size_t m_uploaded;    // Uploaded bytes (for incremental upload)
    int m_generation;
  };
}

#endif // LUMINOUS_TEXTUREGL_HPP
