#include "Luminous/RenderDriver.hpp"
#include "Luminous/OpenGL/RenderDriverGL.hpp"

#include "Radiant/Platform.hpp"

namespace Luminous
{
  /// Select the correct renderdriver for this particular platform
  std::shared_ptr<RenderDriver> RenderDriver::createInstance(unsigned int threadIndex)
  {
#if defined (RADIANT_WINDOWS) || defined (RADIANT_LINUX) || defined (RADIANT_OSX)
    return std::make_shared<RenderDriverGL>(threadIndex);
#else
#   error "createRenderDriver: Unsupported platform"
#endif
  }
}

