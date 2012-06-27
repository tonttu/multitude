#if !defined (LUMINOUS_HARDWAREBUFFER_HPP)
#define LUMINOUS_HARDWAREBUFFER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

namespace Luminous
{
  class HardwareBuffer : public RenderResource
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

    enum LockOptions
    {
      BufferLockOptions_Discard     = (1 << 0),
      BufferLockOptions_Read        = (1 << 1),
      BufferLockOptions_Write       = (1 << 2),
      BufferLockOptions_NoOverwrite = (1 << 3),
      BufferLockOptions_ReadWrite   = BufferLockOptions_Read | BufferLockOptions_Write,
    };

    enum Type
    {
      Vertex,
      Index,
      Constant,
    };

  public:
    LUMINOUS_API HardwareBuffer();
    LUMINOUS_API ~HardwareBuffer();

    LUMINOUS_API void setData(const char * data, size_t size, Usage usage);

    LUMINOUS_API size_t size() const;
    LUMINOUS_API const char * data() const;
    LUMINOUS_API Usage usage() const;

  private:
    friend class VertexAttributeBinding;
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_HARDWAREBUFFER_HPP
