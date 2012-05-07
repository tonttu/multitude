#if !defined (LUMINOUS_HARDWAREBUFFER_HPP)
#define LUMINOUS_HARDWAREBUFFER_HPP

#include "Luminous/Luminous.hpp"

namespace Luminous
{
  /// @todo add map/unmap functionality
  class HardwareBuffer
  {
  public:
    virtual ~HardwareBuffer() {}

    virtual void reallocate(size_t bytes, BufferUsage usage) = 0;
    virtual size_t size() = 0;

    virtual void read(char * data, size_t offset, size_t bytes) const = 0;
    virtual void write(const char * data, size_t offset, size_t bytes) = 0;

    virtual void bind(int threadIndex) = 0;
    virtual void unbind(int threadIndex) = 0;
    
    virtual BufferType type() const = 0;
    virtual BufferUsage usage() const = 0;
  };
}
#endif // LUMINOUS_HARDWAREBUFFER_HPP