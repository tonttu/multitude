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
    HardwareBuffer(RenderResource::Id id, BufferType type, RenderDriver & driver);
    LUMINOUS_API virtual ~HardwareBuffer();

    LUMINOUS_API virtual void reallocate(size_t bytes, BufferUsage usage);
    LUMINOUS_API virtual size_t size() const;

    LUMINOUS_API virtual void read(char * data, size_t offset, size_t bytes) const;
    LUMINOUS_API virtual void write(const char * data, size_t offset, size_t bytes);
    
    LUMINOUS_API virtual BufferType type() const;
    LUMINOUS_API virtual BufferUsage usage() const;

    LUMINOUS_API virtual const char * data() const;
  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_HARDWAREBUFFER_HPP