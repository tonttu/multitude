#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    D() : size(0), data(nullptr) {}
    size_t size;
    const char * data;
  };

  HardwareBuffer::HardwareBuffer(RenderResource::Id id, RenderDriver & driver)
    : RenderResource(id, RT_Buffer, driver)
    , m_d(new HardwareBuffer::D())
  {
  }

  void HardwareBuffer::setData(const char * data, size_t size)
  {
    m_d->data = data;
    m_d->size = size;

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
}