#if !defined (LUMINOUS_VERTEXATTRIBUTEBINDINGGL_HPP)
#define LUMINOUS_VERTEXATTRIBUTEBINDINGGL_HPP

#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/RenderResource.hpp"

namespace Luminous
{
  /// OpenGL specific VertexAttributeBinding using VAOs
  class VertexAttributeBindingGL
    : public VertexAttributeBinding
    , public RenderResource
  {
    typedef VertexAttributeBinding Parent;
  public:
    VertexAttributeBindingGL(RenderResource::Id id, unsigned int threadCount);
    ~VertexAttributeBindingGL();

    // VertexAttributeBinding interface
    virtual void bind(unsigned int threadIndex) OVERRIDE;
    virtual void unbind(unsigned int threadIndex) OVERRIDE;

    // RenderResource interface
    virtual void initializeResources(unsigned int threadIndex) OVERRIDE;
    virtual void deinitializeResources(unsigned int threadIndex) OVERRIDE;
  private:
    class Impl;
    Impl * m_impl;
  };
}
#endif // LUMINOUS_VERTEXATTRIBUTEBINDINGGL_HPP