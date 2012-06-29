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
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  /// Factory class for render resources
  class RenderManager
  {
  public:
    LUMINOUS_API RenderManager(Luminous::RenderDriver & driver);
    LUMINOUS_API ~RenderManager();

    LUMINOUS_API RenderResource::Id createResource(RenderResource * resource);
    LUMINOUS_API void destroyResource(RenderResource::Id id);
    LUMINOUS_API RenderDriver & driver();

    LUMINOUS_API static RenderManager & instance();

    /// * Can't derive proper type from the resource id so we need per-type accessors
    /// * Must return naked pointer since requested ID might not exist anymore (we don't have a weak_ptr/shared_ptr here)
    template <typename T> static T * getResource( RenderResource::Id id );
  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_RENDERMANAGER_HPP
