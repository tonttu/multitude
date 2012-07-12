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

#include "Export.hpp"
#include <Luminous/Luminous.hpp>
#include <Luminous/RenderResource.hpp>

#include <vector>

namespace Luminous
{
  /// Factory class for render resources
  class RenderManager
  {
  public:
    LUMINOUS_API RenderManager();
    LUMINOUS_API ~RenderManager();

    LUMINOUS_API void setDrivers(std::vector<Luminous::RenderDriver*> drivers);

    LUMINOUS_API static RenderResource::Id createResource(RenderResource * resource);
    LUMINOUS_API static void destroyResource(RenderResource::Id id);

    template <typename T> static T * getResource( RenderResource::Id id );
  private:
    LUMINOUS_API static RenderManager & instance();
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_RENDERMANAGER_HPP
