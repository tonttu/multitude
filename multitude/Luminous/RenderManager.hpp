/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
    /// Frame time of the previous rendered frame. This value should be used
    /// to determine if a resource should be expired instead of the value
    /// returned from frameTime(), since this frame has been finished unlike
    /// frameTime() which can still be in process.
    LUMINOUS_API static int lastFrameTime();
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
