/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Luminous/RenderDriver.hpp"
#include "Luminous/RenderDriverGL.hpp"

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

