#include "Luminous/HardwareBufferImpl.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  HardwareBuffer::HardwareBuffer()
    : RenderResource(ResourceType_Buffer)
    , m_d(new HardwareBuffer::D(this))
  {
  }


  HardwareBuffer::~HardwareBuffer()
  {
    delete m_d;
  }

  void HardwareBuffer::setData(const char * data, size_t size, BufferUsage usage)
  {
    m_d->data->data = data;
    m_d->data->size = size;
    m_d->data->usage = usage;

    invalidate();
  }

  const char * HardwareBuffer::data() const
  {
    return m_d->data->data;
  }

  size_t HardwareBuffer::size() const
  {
    return m_d->data->size;
  }

  BufferUsage HardwareBuffer::usage() const
  {
    return m_d->data->usage;
  }
}