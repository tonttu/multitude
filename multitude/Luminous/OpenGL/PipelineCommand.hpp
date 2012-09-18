#ifndef LUMINOUS_OPENGL_PIPELINECOMMAND_HPP
#define LUMINOUS_OPENGL_PIPELINECOMMAND_HPP

#include "RenderTargetGL.hpp"
#include "RenderDriverGL.hpp"

namespace Luminous
{

  /// Interface for render pipeline commands. Pipeline commands are rendering
  /// commands that define segments segments of commands that can not be
  /// re-ordered.
  class PipelineCommand
  {
  public:
    virtual void execute() = 0;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  /// This command clears the render target.
  class CommandClearGL : public PipelineCommand
  {
  public:
    CommandClearGL(Luminous::ClearMask clearMask, const Radiant::Color & clearColor, float clearDepth, int clearStencil);

    virtual void execute();

  private:
    Luminous::ClearMask m_clearMask;
    Radiant::Color m_clearColor;
    float m_clearDepth;
    int m_clearStencil;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  /// This command changes the active render target.
  class CommandChangeRenderTargetGL : public PipelineCommand
  {
  public:
    CommandChangeRenderTargetGL(RenderTargetGL & rt);

    virtual void execute();

  private:
    RenderTargetGL & m_renderTarget;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  /// This command sets the scissor box.
  class CommandScissorGL : public PipelineCommand
  {
  public:
    CommandScissorGL(const Nimble::Recti & rect);

    virtual void execute();

  private:
    Nimble::Recti m_rect;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  /// This command sets the current viewport transformation
  class CommandViewportGL : public PipelineCommand
  {
  public:
    CommandViewportGL(const Nimble::Recti & rect);

    virtual void execute();

  private:
    Nimble::Recti m_rect;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  /// Performs a blit operation from render target bound as READ to render
  /// target bound as DRAW.
  class CommandBlitGL : public PipelineCommand
  {
  public:
    CommandBlitGL(const Nimble::Recti & src, const Nimble::Recti & dst,
                  Luminous::ClearMask mask = Luminous::ClearMask_ColorDepth,
                  Luminous::Texture::Filter filter = Luminous::Texture::Filter_Nearest);

    virtual void execute();

  private:
    Nimble::Recti m_src;
    Nimble::Recti m_dst;
    Luminous::ClearMask m_mask;
    Luminous::Texture::Filter m_filter;
  };
}

#endif
