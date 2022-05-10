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
