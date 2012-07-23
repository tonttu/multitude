#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    D(Type t)
      : size(0)
      , data(nullptr)
      , usage(HardwareBuffer::StaticDraw)
      , type(t)
    {
    }

    size_t size;
    const char * data;
    HardwareBuffer::Usage usage;
    HardwareBuffer::Type type;
  };

  HardwareBuffer::HardwareBuffer(Type type)
    : RenderResource(RenderResource::Buffer)
    , m_d(new HardwareBuffer::D(type))
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

  void HardwareBuffer::setType(Type type)
  {
    m_d->type = type;
  }

  HardwareBuffer::Type HardwareBuffer::type() const
  {
    return m_d->type;
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
