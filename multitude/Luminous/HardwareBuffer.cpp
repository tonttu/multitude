#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    D() : size(0), data(nullptr), usage(BufferUsage_Dynamic), access(BufferAccess_Write) {}
    size_t size;
    const char * data;
    BufferUsage usage;
    BufferAccess access;
  };

  HardwareBuffer::HardwareBuffer(RenderResource::Id id, RenderDriver & driver)
    : RenderResource(id, ResourceType_Buffer, driver)
    , m_d(new HardwareBuffer::D())
  {
  }

  void HardwareBuffer::setData(const char * data, size_t size, BufferUsage usage, BufferAccess access)
  {
    m_d->data = data;
    m_d->size = size;
    m_d->usage = usage;
    m_d->access = access;

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