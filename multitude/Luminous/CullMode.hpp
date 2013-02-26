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

#ifndef CULLMODE_HPP
#define CULLMODE_HPP

#include "RenderDefines.hpp"

namespace Luminous
{

  /// This class defines the culling-mode for rendered primitives
  class CullMode
  {
  public:
    /// Returns the default culling mode. The mode will cull back-facing primitives.
    static CullMode Default() { return CullMode(); }

    LUMINOUS_API CullMode();
    /// Construct a new CullMode
    /// @param enabled is culling enabled or not
    /// @param face which faces are culled
    LUMINOUS_API CullMode(bool enabled, Luminous::Face face);

    /// Check if culling is enabled
    bool enabled() const { return m_enabled; }
    /// Check which faces are culled
    Luminous::Face face() const { return m_face; }

  private:
    bool m_enabled;
    Luminous::Face m_face;
  };

}

#endif // CULLMODE_HPP
