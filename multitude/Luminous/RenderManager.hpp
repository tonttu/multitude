/* COPYRIGHT
 *
 * This file is part of ThreadedRendering.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "ThreadedRendering.hpp" for authors and more details.
 *
 */

#ifndef LUMINOUS_RENDERMANAGER_HPP
#define LUMINOUS_RENDERMANAGER_HPP

/// @cond

#include "Export.hpp"

#include <Radiant/Mutex.hpp>
#include <Luminous/Luminous.hpp>
#include <Luminous/RenderResource.hpp>

#include <vector>

namespace Luminous
{
  /// Factory class for render resources
  class RenderManager
  {
  public:

    LUMINOUS_API static void setDrivers(std::vector<Luminous::RenderDriver*> drivers);

    LUMINOUS_API static RenderResource::Id createResource(RenderResource * resource);
    LUMINOUS_API static void updateResource(RenderResource::Id, RenderResource * resource);
    LUMINOUS_API static void destroyResource(RenderResource::Id id);

    LUMINOUS_API static void addContextArray(ContextArray * contextArray);
    LUMINOUS_API static void removeContextArray(ContextArray * contextArray);
    LUMINOUS_API static unsigned int driverCount();

    /// Current frame time, in tenths of a second since the application was started
    /// This needs to be integer and 32 bit to fit to atomic int, so 0.1s is a nice
    /// precision.
    LUMINOUS_API static int frameTime();
    LUMINOUS_API static void updateFrameTime();

    LUMINOUS_API static void setThreadIndex(unsigned idx);
    LUMINOUS_API static unsigned threadIndex();

    template <typename T> static T * getResource( RenderResource::Id id );

    LUMINOUS_API static Radiant::Mutex & resourceLock();

  private:
    RenderManager();
  };
}

/// @endcond

#endif // LUMINOUS_RENDERMANAGER_HPP
