#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    D()
      : size(0)
      , data(nullptr)
      , usage(HardwareBuffer::StaticDraw)
    {
    };

    size_t size;
    const char * data;
    HardwareBuffer::Usage usage;
  };

  HardwareBuffer::HardwareBuffer()
    : RenderResource(RenderResource::Buffer)
    , m_d(new HardwareBuffer::D())
  {
  }

  HardwareBuffer::~HardwareBuffer()
  {
    delete m_d;
  }

  HardwareBuffer::HardwareBuffer(HardwareBuffer && b)
    : RenderResource(std::move(b))
    , m_d(b.m_d)
  {
    b.m_d = nullptr;
  }

  HardwareBuffer & HardwareBuffer::operator=(HardwareBuffer && b)
  {
    RenderResource::operator=(std::move(b));
    std::swap(m_d, b.m_d);
    return *this;
  }

  void HardwareBuffer::setData(const char * data, size_t size, HardwareBuffer::Usage usage)
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

  HardwareBuffer::Usage HardwareBuffer::usage() const
  {
    return m_d->usage;
  }
}
