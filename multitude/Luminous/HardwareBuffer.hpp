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
    HardwareBuffer(RenderResource::Id id, BufferType type);
    virtual ~HardwareBuffer();

    virtual void reallocate(size_t bytes, BufferUsage usage);
    virtual size_t size() const;

    virtual void read(char * data, size_t offset, size_t bytes) const;
    virtual void write(const char * data, size_t offset, size_t bytes);
    
    virtual BufferType type() const;
    virtual BufferUsage usage() const;

    virtual const char * data() const;
  private:
    class Impl;
    Impl * m_impl;
  };
}
#endif // LUMINOUS_HARDWAREBUFFER_HPP