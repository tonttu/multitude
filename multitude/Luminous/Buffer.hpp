/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_BUFFER_HPP)
#define LUMINOUS_BUFFER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <Radiant/Flags.hpp>

namespace Luminous
{
  /// This class represents a general unformatted linear memory stored on the
  /// graphics card. It can be used to store vertex data, pixel data retrieved
  /// from images or the framebuffer, etc.
  /// GPU correspondent of this class is BufferGL.
  class Buffer : public RenderResource
  {
  public:

    /// Hint indicating the expected application usage pattern of the buffer. A
    /// buffer’s data store is sourced when it is read from as a result of
    /// OpenGL commands which specify images, or invoke shaders accessing
    /// buffer data as a result of drawing commands or compute shader dispatch.
    enum Usage
    {
      /// The data store contents will be speciﬁed once by the application, and
      /// sourced many times
      STATIC_DRAW = GL_STATIC_DRAW,

      /// The data store contents will be speciﬁed once by reading data from
      /// the GL, and queried many times by the application
      STATIC_READ = GL_STATIC_READ,

      /// The data store contents will be speciﬁed once by reading data from
      /// the GL, and sourced many times
      STATIC_COPY = GL_STATIC_COPY,

      /// The data store contents will be speciﬁed once by the application, and
      /// sourced at most a few times.
      STREAM_DRAW = GL_STREAM_DRAW,

      /// The data store contents will be speciﬁed once by reading data from
      /// the GL, and queried at most a few times by the application
      STREAM_READ = GL_STREAM_READ,

      /// The data store contents will be speciﬁed once by reading data from
      /// the GL, and sourced at most a few times
      STREAM_COPY = GL_STREAM_COPY,

      /// The data store contents will be respeciﬁed repeatedly by the
      /// application, and sourced many times
      DYNAMIC_DRAW = GL_DYNAMIC_DRAW,

      /// The data store contents will be respeciﬁed repeatedly by reading data
      /// from the GL, and queried many times by the application
      DYNAMIC_READ = GL_DYNAMIC_READ,

      /// The data store contents will be respeciﬁed repeatedly by reading data
      /// from the GL, and sourced many times
      DYNAMIC_COPY = GL_DYNAMIC_COPY
    };

    /// Map access modifiers
    enum MapAccess
    {
      MAP_READ               = GL_MAP_READ_BIT,
      MAP_WRITE              = GL_MAP_WRITE_BIT,
      MAP_READ_WRITE         = MAP_READ | MAP_WRITE,
      MAP_INVALIDATE_RANGE   = GL_MAP_INVALIDATE_RANGE_BIT,
      MAP_INVALIDATE_BUFFER  = GL_MAP_INVALIDATE_BUFFER_BIT,
      MAP_FLUSH_EXPLICIT     = GL_MAP_FLUSH_EXPLICIT_BIT,
      MAP_UNSYNCHRONIZED     = GL_MAP_UNSYNCHRONIZED_BIT
    };

    /// Buffer object type
    enum Type
    {
      UNKNOWN  = 0,
      VERTEX   = GL_ARRAY_BUFFER,
      INDEX    = GL_ELEMENT_ARRAY_BUFFER,
      UNIFORM  = GL_UNIFORM_BUFFER
    };

  public:
    /// Constructor
    LUMINOUS_API Buffer();
    /// Destructor
    LUMINOUS_API ~Buffer();

    /// Copy constructor
    /// @param b buffer to copy
    LUMINOUS_API Buffer(const Buffer & b);
    /// Assignment operator
    /// @param b buffer to copy
    LUMINOUS_API Buffer & operator=(const Buffer & b);

    /// Move constructor
    /// @param b buffer to move
    LUMINOUS_API Buffer(Buffer && b);
    /// Move assignment operator
    /// @param b buffer to move
    LUMINOUS_API Buffer & operator=(Buffer && b);

    /// Set the buffer contents. Does not copy the data. The pointer must
    /// remain valid as long as the buffer is in use.
    /// @param data pointer to data
    /// @param size size of the data in bytes
    /// @param usage usage hint
    LUMINOUS_API void setData(const void * data, size_t size, Usage usage);

    /// Get the size of the buffer
    /// @return size of the buffer in bytes
    LUMINOUS_API size_t size() const;
    /// Get a pointer to the buffer data
    /// @return data pointer
    LUMINOUS_API const void * data() const;
    /// Get the usage hints for the buffer
    /// @return usage hints
    LUMINOUS_API Usage usage() const;

  private:
    friend class VertexArray;
    class D;
    D * m_d;
  };
  MULTI_FLAGS(Buffer::MapAccess)
}
#endif // LUMINOUS_BUFFER_HPP
