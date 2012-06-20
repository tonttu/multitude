#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    D() : size(0), data(nullptr), usage(BufferUsage_Static_Draw) {}
    size_t size;
    const char * data;
    BufferUsage usage;
  };

  HardwareBuffer::HardwareBuffer()
    : RenderResource(ResourceType_Buffer)
    , m_d(new HardwareBuffer::D())
  {
  }

  void HardwareBuffer::setData(const char * data, size_t size, BufferUsage usage)
  {
    m_d->data = data;
    m_d->size = size;
    m_d->usage = usage;

    invalidate();
  }

  const char * HardwareBuffer::data() const
  {
    return m_d->data;
  }

  size_t HardwareBuffer::size() const
  {
    return m_d->size;
  }

  BufferUsage HardwareBuffer::usage() const
  {
    return m_d->usage;
  }
}