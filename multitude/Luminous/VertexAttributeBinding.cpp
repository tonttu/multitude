#include "Luminous/VertexAttributeBinding.hpp"

#include <algorithm>

namespace Luminous
{
  class VertexAttributeBinding::Impl
  {
  public:
    struct Binding
    {
      Binding(std::shared_ptr<HardwareBuffer> buffer, std::shared_ptr<VertexDescription> description)
        : buffer(buffer)
        , description(description)
      {}

      bool operator==(const std::shared_ptr<HardwareBuffer> & rhs) const { return buffer == rhs; }

      std::shared_ptr<HardwareBuffer> buffer;
      std::shared_ptr<VertexDescription> description;
    };

    typedef std::vector< Binding > Bindings;
    Bindings bindings;

    bool dirty;
  };

  VertexAttributeBinding::VertexAttributeBinding()
    : m_impl(new VertexAttributeBinding::Impl())
  {

  }
  VertexAttributeBinding::~VertexAttributeBinding()
  {
    delete m_impl;
  }

  void VertexAttributeBinding::addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description)
  {
    // Add the binding if it doesn't already exist
    Impl::Bindings::const_iterator it = std::find(m_impl->bindings.begin(), m_impl->bindings.end(), buffer);
    if (it == m_impl->bindings.end()) {
      m_impl->bindings.push_back(Impl::Binding(buffer, description));
      setDirty(true);
    }
  }

  void VertexAttributeBinding::removeBinding(const std::shared_ptr<HardwareBuffer> & buffer)
  {
    Impl::Bindings::iterator it = std::find(m_impl->bindings.begin(), m_impl->bindings.end(), buffer);
    if (it != m_impl->bindings.end()) {
      m_impl->bindings.erase( it );
      setDirty(true);
    }
  }

  void VertexAttributeBinding::clear()
  {
    if (!m_impl->bindings.empty()) {
      m_impl->bindings.clear();
      setDirty(true);
    }
  }

  void VertexAttributeBinding::setDirty(bool dirty)
  {
    m_impl->dirty = dirty;
  }

  bool VertexAttributeBinding::dirty() const
  {
    return m_impl->dirty;
  }
}