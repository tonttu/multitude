#if !defined (LUMINOUS_VERTEXATTRIBUTEBINDING_HPP)
#define LUMINOUS_VERTEXATTRIBUTEBINDING_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/VertexDescription.hpp"

#include <Radiant/RefPtr.hpp>
#include <vector>

namespace Luminous
{
  class VertexAttributeBinding
    : public RenderResource
  {
  public:
    struct Binding
    {
      std::shared_ptr<HardwareBuffer> buffer;
      std::shared_ptr<VertexDescription> description;
    };
  public:
    VertexAttributeBinding(RenderResource::Id id);
    ~VertexAttributeBinding();

    void addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description);
    void removeBinding(const std::shared_ptr<HardwareBuffer> & buffer);
    void clear();

    size_t bindingCount() const;
    const Binding & binding(size_t index) const;
  private:
    class Impl;
    Impl * m_impl;
  };
}

#endif // LUMINOUS_VERTEXATTRIBUTEBINDING_HPP
