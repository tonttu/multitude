#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/VertexDescription.hpp"

#include <algorithm>

namespace Luminous
{
  class VertexAttributeBinding::D
  {
  public:
    D() : indexBuffer(0) {}
    typedef std::vector< VertexAttributeBinding::Binding > Bindings;
    Bindings bindings;
    RenderResource::Id indexBuffer;
  };

  //bool operator==(const VertexAttributeBinding::Binding & lhs, const std::shared_ptr<HardwareBuffer> & rhs) { return lhs.buffer == rhs.resourceId(); }

  VertexAttributeBinding::VertexAttributeBinding()
    : RenderResource(RenderResource::VertexArray)
    , m_d(new VertexAttributeBinding::D())
  {
  }

  VertexAttributeBinding::~VertexAttributeBinding()
  {
    delete m_d;
  }

  void VertexAttributeBinding::addBinding(const Luminous::HardwareBuffer & vertexBuffer, const Luminous::VertexDescription & description)
  {
    // Add the binding if it doesn't already exist
    D::Bindings::const_iterator it = std::find(m_d->bindings.begin(), m_d->bindings.end(), vertexBuffer.resourceId());
    if (it == m_d->bindings.end()) {
      Binding binding = { vertexBuffer.resourceId(), description.resourceId() };
      m_d->bindings.push_back(binding);
      invalidate();
    }
  }

  void VertexAttributeBinding::setIndexBuffer(const Luminous::HardwareBuffer & indexBuffer)
  {
    m_d->indexBuffer = indexBuffer.resourceId();
  }

  void VertexAttributeBinding::removeBinding(const Luminous::HardwareBuffer & buffer)
  {
    D::Bindings::iterator it = std::find(m_d->bindings.begin(), m_d->bindings.end(), buffer.resourceId());
    if (it != m_d->bindings.end()) {
      m_d->bindings.erase( it );
      invalidate();
    }
  }

  void VertexAttributeBinding::clear()
  {
    if (!m_d->bindings.empty()) {
      m_d->bindings.clear();
      invalidate();
    }
  }

  size_t VertexAttributeBinding::bindingCount() const
  {
    return m_d->bindings.size();
  }

  const VertexAttributeBinding::Binding & VertexAttributeBinding::binding(size_t index) const
  {
    assert( index < bindingCount() );
    return m_d->bindings[index];
  }

  RenderResource::Id VertexAttributeBinding::indexBuffer() const
  {
    return m_d->indexBuffer;
  }
}