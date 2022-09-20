/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef MASKGUARD_HPP
#define MASKGUARD_HPP

#include "Export.hpp"
#include "RenderContext.hpp"

#include "Patterns/NotCopyable.hpp"

namespace Luminous
{

  /// This class provides a simple guard for setting the active masking area
  /// for rendering. It will automatically pop in its destructor, so the user
  /// does not need to remember to clean up manually. All rendering performed
  /// while the guard is alive is clipped to the rectangle given to it.
  class LUMINOUS_API MaskGuard : public Patterns::NotCopyable
  {
  public:
    /// Construct new mask guard
    /// @param rc render context
    /// @param rect mask rectangle
    MaskGuard(RenderContext & rc, const Nimble::Rect &rect);
    /// Move constructor
    /// @param g guard to move
    MaskGuard(MaskGuard && g);
    /// Destructor. Automatically removes the mask.
    ~MaskGuard();

  private:
    void renderMask();

    RenderContext* m_rc;
    Nimble::Rect m_rect;
    Luminous::StencilMode m_originalStencilMode;
  };

}

#endif // MASKGUARD_HPP
