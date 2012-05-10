#include "Luminous/RenderContextImmediate.hpp"
#include "Luminous/RenderDriver.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/GLContext.hpp"

namespace Luminous
{
  //////////////////////////////////////////////////////////////////////////
  /// Rendercontext implementation details
  class RenderContextImmediate::Impl
  {
  public:
    Impl(const std::shared_ptr<GLContext> & deviceContext, const std::shared_ptr<RenderDriver> & driver)
      : context(deviceContext)
      , driver(driver)
      , threadIndex(0)
      , frame(0)
      , fps(0.f)
    {
    }

    std::shared_ptr<GLContext> context;
    std::shared_ptr<RenderDriver> driver;

    /// @todo Use threadIndex
    unsigned int threadIndex;
    size_t frame;
    float fps;
  };

  //////////////////////////////////////////////////////////////////////////
  RenderContextImmediate::RenderContextImmediate(const std::shared_ptr<GLContext> & deviceContext, const std::shared_ptr<RenderDriver> & driver)
    : m_impl(std::make_shared<RenderContextImmediate::Impl>(deviceContext, driver))
  {
  }

  /// Returns the current frame
  size_t RenderContextImmediate::frame() const
  {
    /// @todo should get this from the device context
    return m_impl->frame;
  }

  /// Returns the framerate (frames per second)
  float RenderContextImmediate::framerate() const
  {
    /// @todo should get this from the device context
    return m_impl->fps;
  }

  /// Binds a set of vertex buffers and their vertexdescriptions
  void RenderContextImmediate::bind(VertexAttributeBinding & binding)
  {
    
  }

  /// Issue a draw-call
  void RenderContextImmediate::draw(PrimitiveType type, size_t offset, size_t primitiveCount)
  {
    /// @todo use drawIndexed if an index buffer is active
    m_impl->driver->draw(type, offset, primitiveCount);
  }
}