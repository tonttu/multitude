#if !defined (LUMINOUS_HARDWAREBUFFER_HPP)
#define LUMINOUS_HARDWAREBUFFER_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

namespace Luminous
{
  class HardwareBuffer : public RenderResource
  {
  public:
    LUMINOUS_API HardwareBuffer();

    LUMINOUS_API void setData(const char * data, size_t size, BufferUsage usage);

    LUMINOUS_API size_t size() const;
    LUMINOUS_API const char * data() const;
    LUMINOUS_API BufferUsage usage() const;
  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_HARDWAREBUFFER_HPP
