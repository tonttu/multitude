#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    D(BufferType type, BufferUsage usage)
      : type(type)
      , usage(usage)
    {
    }

    std::vector<char> data;
    BufferType type;
    BufferUsage usage;
  };

  HardwareBuffer::HardwareBuffer(RenderResource::Id id, BufferType type, RenderDriver & driver)
    : RenderResource(id, RT_Buffer, driver)
    , m_d(new HardwareBuffer::D(type, BU_Unknown))
  {
  }

  HardwareBuffer::~HardwareBuffer()
  {
    delete m_d;
  }

  void HardwareBuffer::reallocate(size_t bytes, BufferUsage usage)
  {
    m_d->data.resize(bytes);
    m_d->usage = usage;
  }

  size_t HardwareBuffer::size() const
  {
    return m_d->data.size();
  }

  void HardwareBuffer::read(char * data, size_t offset, size_t bytes) const
  {
    assert(offset + bytes <= m_d->data.size());
    const char * start = m_d->data.data() + offset;
    std::copy(start, start + bytes, data);
  }

  void HardwareBuffer::write(const char * data, size_t offset, size_t bytes)
  {
    assert(offset + bytes <= m_d->data.size());
    char * start = m_d->data.data() + offset;
    std::copy(data, data + bytes, start);

    // Update version
    invalidate();
  }

  BufferType HardwareBuffer::type() const
  {
    return m_d->type;
  }

  BufferUsage HardwareBuffer::usage() const
  {
    return m_d->usage;
  }

  const char * HardwareBuffer::data() const
  {
    return m_d->data.data();
  }
}