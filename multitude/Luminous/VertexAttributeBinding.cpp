#include "Luminous/VertexAttributeBinding.hpp"

#include <algorithm>

namespace Luminous
{
  class VertexAttributeBinding::D
  {
  public:
    typedef std::vector< VertexAttributeBinding::Binding > Bindings;
    Bindings bindings;
  };

  bool operator==(const VertexAttributeBinding::Binding & lhs, const std::shared_ptr<HardwareBuffer> & rhs) { return lhs.buffer == rhs; }

  VertexAttributeBinding::VertexAttributeBinding()
    : RenderResource(ResourceType_VertexArray)
    , m_d(new VertexAttributeBinding::D())
  {
  }

  VertexAttributeBinding::~VertexAttributeBinding()
  {
    delete m_d;
  }

  void VertexAttributeBinding::addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description)
  {
    // Add the binding if it doesn't already exist
    D::Bindings::const_iterator it = std::find(m_d->bindings.begin(), m_d->bindings.end(), buffer);
    if (it == m_d->bindings.end()) {
      Binding binding;
      binding.buffer = buffer;
      binding.description = description;
      m_d->bindings.push_back(binding);
      
      invalidate();
    }
  }

  void VertexAttributeBinding::removeBinding(const std::shared_ptr<HardwareBuffer> & buffer)
  {
    D::Bindings::iterator it = std::find(m_d->bindings.begin(), m_d->bindings.end(), buffer);
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
}