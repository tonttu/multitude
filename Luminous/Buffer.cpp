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
#include "Luminous/ContextArray.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class Buffer::D
  {
  public:
    size_t bufferSize = 0;
    size_t dataSize = 0;
    const void * data = nullptr;
    Buffer::Usage usage = Buffer::STATIC_DRAW;
    ContextArrayT<Buffer::DirtyRegion> dirtyRegions;
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

  void Buffer::setData(const void *data, size_t dataSize, Buffer::Usage usage, size_t bufferSize)
  {
    m_d->data = data;
    m_d->dataSize = dataSize;
    m_d->bufferSize = bufferSize == 0 ? dataSize : bufferSize;
    m_d->usage = usage;

    for (DirtyRegion & d: m_d->dirtyRegions)
      d = DirtyRegion();

    invalidate();
  }

  const void * Buffer::data() const
  {
    return m_d->data;
  }

  size_t Buffer::dataSize() const
  {
    return m_d->dataSize;
  }

  size_t Buffer::bufferSize() const
  {
    return m_d->bufferSize;
  }

  Buffer::Usage Buffer::usage() const
  {
    return m_d->usage;
  }

  Buffer::DirtyRegion Buffer::takeDirtyRegion(unsigned int threadIndex) const
  {
    assert(threadIndex < m_d->dirtyRegions.size());
    DirtyRegion r;
    std::swap(r, m_d->dirtyRegions[threadIndex]);
    return r;
  }

  void Buffer::invalidateRegion(size_t offset, size_t size)
  {
    m_d->dataSize = std::max(m_d->dataSize, offset + size);
    for (DirtyRegion & dirty: m_d->dirtyRegions) {
      if (dirty.dataBegin == dirty.dataEnd) {
        dirty.dataBegin = offset;
        dirty.dataEnd = offset + size;
      } else {
        dirty.dataBegin = std::min(dirty.dataBegin, offset);
        dirty.dataEnd = std::max(dirty.dataEnd, offset + size);
      }
    }
  }
}
