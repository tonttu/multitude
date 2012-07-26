#if !defined (LUMINOUS_BUFFER_HPP)
#define LUMINOUS_BUFFER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <Radiant/Flags.hpp>

#ifdef RADIANT_OSX
// Required for MapAccess enums (which nobody seems to use?)
#include <OpenGL/gl3.h>
#endif

namespace Luminous
{
  class Buffer : public RenderResource
  {
  public:
    enum Usage
    {
      StaticDraw = GL_STATIC_DRAW,
      StaticRead = GL_STATIC_READ,
      StaticCopy = GL_STATIC_COPY,

      StreamDraw = GL_STREAM_DRAW,
      StreamRead = GL_STREAM_READ,
      StreamCopy = GL_STREAM_COPY,

      DynamicDraw = GL_DYNAMIC_DRAW,
      DynamicRead = GL_DYNAMIC_READ,
      DynamicCopy = GL_DYNAMIC_COPY,
    };

    enum MapAccess
    {
      MapRead               = GL_MAP_READ_BIT,
      MapWrite              = GL_MAP_WRITE_BIT,
      MapReadWrite          = MapRead | MapWrite,
      MapInvalidateRange    = GL_MAP_INVALIDATE_RANGE_BIT,
      MapInvalidateBuffer   = GL_MAP_INVALIDATE_BUFFER_BIT,
      MapFlushExplicit      = GL_MAP_FLUSH_EXPLICIT_BIT,
      MapUnsynchronized     = GL_MAP_UNSYNCHRONIZED_BIT,
    };

    enum Type
    {
      Unknown  = 0,
      Vertex   = GL_ARRAY_BUFFER,
      Index    = GL_ELEMENT_ARRAY_BUFFER,
      Uniform  = GL_UNIFORM_BUFFER,
    };

  public:
    LUMINOUS_API Buffer(Type type = Unknown);
    LUMINOUS_API ~Buffer();

    LUMINOUS_API Buffer(Buffer && b);
    LUMINOUS_API Buffer & operator=(Buffer && b);

    LUMINOUS_API void setData(const char * data, size_t size, Usage usage);

    LUMINOUS_API void setType(Type type);
    LUMINOUS_API Type type() const;

    LUMINOUS_API size_t size() const;
    LUMINOUS_API const char * data() const;
    LUMINOUS_API Usage usage() const;

  private:
    friend class VertexAttributeBinding;
    class D;
    D * m_d;
  };
  MULTI_FLAGS(Buffer::MapAccess)
}
#endif // LUMINOUS_BUFFER_HPP
