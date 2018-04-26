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

  /// This class represents the Buffer class in GPU memory.
  /// @sa Buffer
  class BufferGL : public ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state
    /// @param buffer buffer object in CPU memory
    LUMINOUS_API BufferGL(StateGL & state, const Buffer & buffer);
    LUMINOUS_API BufferGL(StateGL & state, Buffer::Usage usage);
    /// Destructor
    LUMINOUS_API ~BufferGL();

    /// Move constructor
    /// @param t buffer to move
    LUMINOUS_API BufferGL(BufferGL && t);
    /// Move assignment operator
    /// @param t buffer to move
    LUMINOUS_API BufferGL & operator=(BufferGL && t);

    /// Bind the buffer to the specified type
    /// @param type type to bind to
    LUMINOUS_API void bind(Buffer::Type type);

    /// Specify buffer data. This function will upload the contents of the CPU
    /// buffer to GPU memory if needed.
    /// @param buffer
    /// @param type buffer type
    LUMINOUS_API void upload(const Buffer &buffer, Buffer::Type type);

    /// Specify buffer data. This function can be used to upload data to the
    /// GPU.
    /// @param type buffer type
    /// @param offset offset in bytes to start uploading from
    /// @param length number of bytes to upload
    /// @param data pointer to data to upload from
    LUMINOUS_API void upload(Buffer::Type type, int offset, std::size_t length, const void * data);

    /// Map a range of the buffer data store. This functions maps the buffer address space
    /// to GPU. The contents of the buffer can then directly be read/written to
    /// relative to the returned pointer.
    /// @param type buffer type
    /// @param offset offset in bytes to start the mapping from
    /// @param length number of bytes to map
    /// @param access access flags
    LUMINOUS_API void * map(Buffer::Type type, int offset, std::size_t length, Radiant::FlagsT<Buffer::MapAccess> access);
    /// Unmaps a buffer range.
    /// @param type buffer type
    /// @param offset offset in bytes to start unmapping from
    /// @param length number of bytes to unmap
    LUMINOUS_API void unmap(Buffer::Type type, int offset = 0, std::size_t length = std::size_t(-1));

  private:
    void allocate(Buffer::Type type);

  private:
    Buffer::Usage m_usage;
    size_t m_size;        // Size (in bytes)
    size_t m_allocatedSize;
    int m_generation;
    Radiant::FlagsT<Buffer::MapAccess> m_mappedAccess;
  };
}

#endif // LUMINOUS_TEXTUREGL_HPP
