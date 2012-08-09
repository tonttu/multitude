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

}

#endif
