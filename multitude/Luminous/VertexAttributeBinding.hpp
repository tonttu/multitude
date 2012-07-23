#if !defined (LUMINOUS_VERTEXATTRIBUTEBINDING_HPP)
#define LUMINOUS_VERTEXATTRIBUTEBINDING_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/VertexDescription.hpp"

#include <Radiant/RefPtr.hpp>
#include <vector>

namespace Luminous
{
  /// VAO
  class VertexAttributeBinding
    : public RenderResource
  {
  public:
    struct Binding
    {
      RenderResource::Id buffer;
      Luminous::VertexDescription description;
      bool operator==(RenderResource::Id id) const { return buffer==id; }
    };

  public:
    LUMINOUS_API VertexAttributeBinding();
    LUMINOUS_API ~VertexAttributeBinding();

    LUMINOUS_API VertexAttributeBinding(VertexAttributeBinding && b);
    LUMINOUS_API VertexAttributeBinding & operator=(VertexAttributeBinding && b);

    LUMINOUS_API void addBinding(const Luminous::HardwareBuffer & vertexBuffer, const Luminous::VertexDescription & description);
    LUMINOUS_API void setIndexBuffer(const Luminous::HardwareBuffer & indexBuffer);
    LUMINOUS_API void removeBinding(const Luminous::HardwareBuffer & buffer);
    LUMINOUS_API void clear();

    LUMINOUS_API size_t bindingCount() const;
    LUMINOUS_API const Binding & binding(size_t index) const;
    LUMINOUS_API RenderResource::Id indexBuffer() const;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_VERTEXATTRIBUTEBINDING_HPP
