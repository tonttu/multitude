#include "MaskGuard.hpp"

namespace Luminous
{

  MaskGuard::MaskGuard(RenderContext &rc, const Nimble::Rect & rect)
    : m_rc(&rc)
    , m_rect(rect)
  {
    size_t depth = rc.currentClipMaskDepth();

    rc.pushClipMaskStack(depth + 1);

    // Assuming 8-bit stencil and no other stencil operations
    assert(depth < 256);

    rc.setRenderBuffers(false, false, true);

    // Setup stencil mode to apply the mask
    StencilMode mode;
    mode.setFunction(FRONT_AND_BACK, StencilMode::Never, depth, 0xff);
    mode.setOperation(FRONT_AND_BACK, StencilMode::Increment, StencilMode::Keep, StencilMode::Keep);

    rc.setStencilMode(mode);

    // Render the mask to update stencil buffer
    renderMask();

    // Setup stencil mode to render content using the mask
    rc.setRenderBuffers(true, true, true);

    mode.setFunction(FRONT_AND_BACK, StencilMode::Less, depth, 0xff);
    mode.setOperation(FRONT_AND_BACK, StencilMode::Keep, StencilMode::Keep, StencilMode::Keep);

    rc.setStencilMode(mode);
  }

  MaskGuard::~MaskGuard()
  {
    if(m_rc) {

      m_rc->popClipMaskStack();

      size_t depth = m_rc->currentClipMaskDepth();

      // Setup stencil mode to clear the mask
      m_rc->setRenderBuffers(false, false, true);

      StencilMode mode;
      mode.setFunction(FRONT_AND_BACK, StencilMode::Never, depth, 0xff);
      mode.setOperation(FRONT_AND_BACK, StencilMode::Decrement, StencilMode::Keep, StencilMode::Keep);

      m_rc->setStencilMode(mode);

      // Render the mask to clear it
      renderMask();

      m_rc->setRenderBuffers(true, true, true);

      // Restore stencil mode
      if(depth > 0) {
        mode.setFunction(FRONT_AND_BACK, StencilMode::LessEqual, depth, 0xff);
        mode.setOperation(FRONT_AND_BACK, StencilMode::Keep, StencilMode::Keep, StencilMode::Keep);

        m_rc->setStencilMode(mode);
      } else {
        m_rc->setStencilMode(StencilMode::Default());
      }
    }
  }

  void MaskGuard::renderMask()
  {
    Luminous::Style style;
    style.setFillColor(1.f, 1.f, 1.f, 1.f);
    m_rc->drawRect(m_rect, style);
  }

  MaskGuard::MaskGuard(MaskGuard && g)
  {
    m_rc = g.m_rc;
    m_rect = g.m_rect;

    g.m_rc = nullptr;
  }


}
