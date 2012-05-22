#if !defined (LUMINOUS_HARDWAREBUFFER_HPP)
#define LUMINOUS_HARDWAREBUFFER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

namespace Luminous
{
  /// @todo add map/unmap functionality
  class HardwareBuffer : public RenderResource
  {
  public:
    LUMINOUS_API HardwareBuffer(RenderResource::Id id, BufferType type, RenderDriver & driver);
    LUMINOUS_API ~HardwareBuffer();

    LUMINOUS_API void reallocate(size_t bytes, BufferUsage usage);
    LUMINOUS_API size_t size() const;

    LUMINOUS_API void read(char * data, size_t offset, size_t bytes) const;
    LUMINOUS_API void write(const char * data, size_t offset, size_t bytes);

    LUMINOUS_API BufferType type() const;
    LUMINOUS_API BufferUsage usage() const;

    LUMINOUS_API const char * data() const;
  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_HARDWAREBUFFER_HPP
