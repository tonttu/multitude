#if !defined (LUMINOUS_RENDERCONTEXTIMMEDIATE_HPP)
#define LUMINOUS_RENDERCONTEXTIMMEDIATE_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderContext2.hpp"
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  class RenderContextImmediate : public RenderContext2
  {
  public:
    RenderContextImmediate(const std::shared_ptr<RenderDriver> & driver);

    /// Returns the current frame
    virtual size_t frame() const OVERRIDE;
    /// Returns the framerate (frames per second)
    virtual float framerate() const OVERRIDE;

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_RENDERCONTEXTIMMEDIATE_HPP
