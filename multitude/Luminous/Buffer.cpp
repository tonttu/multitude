/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/Buffer.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class Buffer::D
  {
  public:
    D()
      : size(0)
      , data(nullptr)
      , usage(Buffer::STATIC_DRAW)
    {
    }

    size_t size;
    const void * data;
    Buffer::Usage usage;
  };

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  Buffer::Buffer()
    : RenderResource(RenderResource::Buffer)
    , m_d(new Buffer::D())
  {
  }

  Buffer::~Buffer()
  {
    delete m_d;
  }

  Buffer::Buffer(const Buffer & b)
    : RenderResource(b)
    , m_d(new Buffer::D(*b.m_d))
  {
  }

  Buffer & Buffer::operator=(const Buffer & b)
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

  void Buffer::setData(const void *data, size_t size, Buffer::Usage usage)
  {
    m_d->data = data;
    m_d->size = size;
    m_d->usage = usage;

    invalidate();
  }

  const void * Buffer::data() const
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
