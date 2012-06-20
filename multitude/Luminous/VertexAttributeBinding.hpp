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
    LUMINOUS_API VertexAttributeBinding();
    LUMINOUS_API ~VertexAttributeBinding();

    LUMINOUS_API void addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description);
    LUMINOUS_API void removeBinding(const std::shared_ptr<HardwareBuffer> & buffer);
    LUMINOUS_API void clear();

    LUMINOUS_API size_t bindingCount() const;
    LUMINOUS_API const Binding & binding(size_t index) const;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_VERTEXATTRIBUTEBINDING_HPP
