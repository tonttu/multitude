/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/VertexArray.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/VertexDescription.hpp"

#include <algorithm>
#include <cassert>

namespace Luminous
{
  class VertexArray::D
  {
  public:
    D() : indexBuffer(0) {}
    typedef std::vector< VertexArray::Binding > Bindings;
    Bindings bindings;
    RenderResource::Id indexBuffer;
  };

  VertexArray::VertexArray()
    : RenderResource(RenderResource::VertexArray)
    , m_d(new VertexArray::D())
  {
  }

  VertexArray::~VertexArray()
  {
    delete m_d;
  }

  VertexArray::VertexArray(VertexArray && b)
    : RenderResource(std::move(b))
    , m_d(b.m_d)
  {
    b.m_d = nullptr;
  }

  VertexArray & VertexArray::operator=(VertexArray && b)
  {
    RenderResource::operator=(std::move(b));
    std::swap(m_d, b.m_d);
    return *this;
  }

  void VertexArray::addBinding(const Luminous::Buffer & vertexBuffer, const Luminous::VertexDescription & description)
  {
    // Add the binding if it doesn't already exist
    D::Bindings::const_iterator it = std::find(m_d->bindings.begin(), m_d->bindings.end(), vertexBuffer.resourceId());
    if (it == m_d->bindings.end()) {
      Binding binding;
      binding.buffer = vertexBuffer.resourceId();
      binding.description = description;
      m_d->bindings.push_back(binding);
      invalidate();
    }
  }

  void VertexArray::setIndexBuffer(const Luminous::Buffer & indexBuffer)
  {
    m_d->indexBuffer = indexBuffer.resourceId();
    invalidate();
  }

  void VertexArray::removeBinding(const Luminous::Buffer & buffer)
  {
    D::Bindings::iterator it = std::find(m_d->bindings.begin(), m_d->bindings.end(), buffer.resourceId());
    if (it != m_d->bindings.end()) {
      m_d->bindings.erase( it );
      invalidate();
    }
  }

  void VertexArray::clear()
  {
    if (!m_d->bindings.empty()) {
      m_d->bindings.clear();
      invalidate();
    }
  }

  size_t VertexArray::bindingCount() const
  {
    return m_d->bindings.size();
  }

  const VertexArray::Binding & VertexArray::binding(size_t index) const
  {
    assert( index < bindingCount() );
    return m_d->bindings[index];
  }

  RenderResource::Id VertexArray::indexBuffer() const
  {
    return m_d->indexBuffer;
  }
}
