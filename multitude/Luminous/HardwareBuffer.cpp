#include "Luminous/HardwareBuffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBuffer::Impl
  {
  public:
    Impl(BufferType type, BufferUsage usage)
      : type(type)
      , usage(usage)
    {
    }

    std::vector<char> data;
    BufferType type;
    BufferUsage usage;
  };

  HardwareBuffer::HardwareBuffer(RenderResource::Id id, BufferType type)
    : RenderResource(id)
    , m_impl(new HardwareBuffer::Impl(type, BU_Unknown))
  {
  }

  HardwareBuffer::~HardwareBuffer()
  {
    delete m_impl;
  }

  void HardwareBuffer::reallocate(size_t bytes, BufferUsage usage)
  {
    m_impl->data.resize(bytes);
    m_impl->usage = usage;
  }

  size_t HardwareBuffer::size() const
  {
    return m_impl->data.size();
  }

  void HardwareBuffer::read(char * data, size_t offset, size_t bytes) const
  {
    assert(offset + bytes < m_impl->data.size());
    const char * start = &(*(m_impl->data.begin() + offset));
    std::copy(start, start + bytes, data);
  }

  void HardwareBuffer::write(const char * data, size_t bytes, size_t offset)
  {
    assert(offset + bytes < m_impl->data.size());
    char * start = &(*(m_impl->data.begin() + offset));
    std::copy(data, data + bytes, start);

    // Update version
    invalidate();
  }

  BufferType HardwareBuffer::type() const
  {
    return m_impl->type;
  }

  BufferUsage HardwareBuffer::usage() const
  {
    return m_impl->usage;
  }

  const char * HardwareBuffer::data() const
  {
    return m_impl->data.data();
  }
}