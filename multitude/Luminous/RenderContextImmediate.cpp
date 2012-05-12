#include "Luminous/RenderContextImmediate.hpp"
#include "Luminous/RenderDriver.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/GLContext.hpp"

namespace Luminous
{
  //////////////////////////////////////////////////////////////////////////
  /// Rendercontext implementation details
  class RenderContextImmediate::D
  {
  public:
    D(const std::shared_ptr<GLContext> & deviceContext, const std::shared_ptr<RenderDriver> & driver)
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
    : m_d(new D(deviceContext, driver))
  {
  }

  /// Returns the current frame
  size_t RenderContextImmediate::frame() const
  {
    /// @todo should get this from the device context
    return m_d->frame;
  }

  /// Returns the framerate (frames per second)
  float RenderContextImmediate::framerate() const
  {
    /// @todo should get this from the device context
    return m_d->fps;
  }
}