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
    VertexAttributeBindingGL(int threadCount);
    ~VertexAttributeBindingGL();

    // VertexAttributeBinding interface
    virtual void addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description) OVERRIDE;
    virtual void removeBinding(const std::shared_ptr<HardwareBuffer> & buffer) OVERRIDE;
    virtual void clear() OVERRIDE;

    virtual void bind(int threadIndex) OVERRIDE;
    virtual void unbind(int threadIndex) OVERRIDE;

    // RenderResource interface
    virtual void initializeResources(int threadIndex);
    virtual void updateResources(int threadIndex);
    virtual void deinitializeResources(int threadIndex);
  private:
    class Impl;
    Impl * m_impl;
  };
}
#endif // LUMINOUS_VERTEXATTRIBUTEBINDINGGL_HPP