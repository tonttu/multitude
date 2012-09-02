#include "Luminous/Buffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class Buffer::D
  {
  public:
    D(Type t)
      : size(0)
      , data(nullptr)
      , usage(Buffer::StaticDraw)
      , type(t)
    {
    }

    size_t size;
    const char * data;
    Buffer::Usage usage;
  };

  Buffer::Buffer(Type type)
    : RenderResource(RenderResource::Buffer)
    , m_d(new Buffer::D(type))
  {
  }

  Buffer::~Buffer()
  {
    delete m_d;
  }

  Buffer::Buffer(Buffer & b)
    : RenderResource(b)
    , m_d(new Buffer::D(*b.m_d))
  {
  }

  Buffer & Buffer::operator=(Buffer & b)
  {
    if(this != &b)
    {
      RenderResource::operator=(b);
      assert(b.m_d);
      *m_d = *b.m_d;
    }
    return *this;
  }

  Buffer::Buffer(Buffer && b)
    : RenderResource(std::move(b))
    , m_d(b.m_d)
  {
    b.m_d = nullptr;
  }

  Buffer & Buffer::operator=(Buffer && b)
  {
    RenderResource::operator=(std::move(b));
    std::swap(m_d, b.m_d);
    return *this;
  }

  void Buffer::setData(const char * data, size_t size, Buffer::Usage usage)
  {
    m_d->data = data;
    m_d->size = size;
    m_d->usage = usage;

    invalidate();
  }

  const char * Buffer::data() const
  {
    return m_d->data;
  }

  size_t Buffer::size() const
  {
    return m_d->size;
  }

  Buffer::Usage Buffer::usage() const
  {
    return m_d->usage;
  }
}
