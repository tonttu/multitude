#if !defined (LUMINOUS_VERTEX_ARRAY_HPP)
#define LUMINOUS_VERTEX_ARRAY_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/VertexDescription.hpp"

#include <memory>
#include <vector>

namespace Luminous
{

  /// This class abstracts OpenGL VertexArrayObjects.
  /// @todo implement copying (note bindings)
  class VertexArray
    : public RenderResource
    , public Patterns::NotCopyable
  {
  public:
    struct Binding
    {
      RenderResource::Id buffer;
      Luminous::VertexDescription description;
      bool operator==(RenderResource::Id id) const { return buffer==id; }
    };

  public:
#ifdef RADIANT_DELETED_CONSTRUCTORS
    VertexArray(const VertexArray &) = delete;
#endif
    LUMINOUS_API VertexArray();
    LUMINOUS_API ~VertexArray();

    LUMINOUS_API VertexArray(VertexArray && b);
    LUMINOUS_API VertexArray & operator=(VertexArray && b);

    LUMINOUS_API void addBinding(const Luminous::Buffer & vertexBuffer, const Luminous::VertexDescription & description);
    LUMINOUS_API void setIndexBuffer(const Luminous::Buffer & indexBuffer);
    LUMINOUS_API void removeBinding(const Luminous::Buffer & buffer);
    LUMINOUS_API void clear();

    LUMINOUS_API size_t bindingCount() const;
    LUMINOUS_API const Binding & binding(size_t index) const;
    LUMINOUS_API RenderResource::Id indexBuffer() const;
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_VERTEX_ARRAY_HPP
