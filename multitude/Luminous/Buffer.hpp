/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
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
    enum Usage
    {
      STATIC_DRAW = GL_STATIC_DRAW,
      STATIC_READ = GL_STATIC_READ,
      STATIC_COPY = GL_STATIC_COPY,

      STREAM_DRAW = GL_STREAM_DRAW,
      STREAM_READ = GL_STREAM_READ,
      STREAM_COPY = GL_STREAM_COPY,

      DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
      DYNAMIC_READ = GL_DYNAMIC_READ,
      DYNAMIC_COPY = GL_DYNAMIC_COPY
    };

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

    enum Type
    {
      UNKNOWN  = 0,
      VERTEX   = GL_ARRAY_BUFFER,
      INDEX    = GL_ELEMENT_ARRAY_BUFFER,
      UNIFORM  = GL_UNIFORM_BUFFER
    };

  public:
    LUMINOUS_API Buffer();
    LUMINOUS_API ~Buffer();

    LUMINOUS_API Buffer(Buffer & b);
    LUMINOUS_API Buffer & operator=(Buffer & b);

    LUMINOUS_API Buffer(Buffer && b);
    LUMINOUS_API Buffer & operator=(Buffer && b);

    LUMINOUS_API void setData(const void * data, size_t size, Usage usage);

    LUMINOUS_API size_t size() const;
    LUMINOUS_API const void * data() const;
    LUMINOUS_API Usage usage() const;

  private:
    friend class VertexArray;
    class D;
    D * m_d;
  };
  MULTI_FLAGS(Buffer::MapAccess)
}
#endif // LUMINOUS_BUFFER_HPP
