#ifndef LUMINOUS_RENDER_TARGET_HPP
#define LUMINOUS_RENDER_TARGET_HPP

#include "FramebufferResource.hpp"

#include <Radiant/RefPtr.hpp>

#include <Nimble/Rect.hpp>

#include <map>
#include <stack>

namespace Luminous
{
  class RenderTargetObject;
  class RenderContext;

  /// Simple render-to-texture provider meant to replace RenderContext::getTemporaryFBO() functionality.
  /// Only supports single draw buffer currently
  class RenderTargetManager
  {
  public:
    RenderTargetManager(Luminous::RenderContext & rc);

    RenderTargetObject pushRenderTarget(Nimble::Vector2 size, float scale);
    Luminous::Texture2D & popRenderTarget(RenderTargetObject & trt);

  private:
    struct RenderTargetState {
      bool inUse;
      FramebufferResource resource;
    };

    std::shared_ptr<RenderTargetState> allocateNewTexture(size_t extent);
    std::shared_ptr<RenderTargetState> findAvailableTexture(size_t extent);

    typedef std::multimap<size_t, std::shared_ptr<RenderTargetState> > Textures;

    Textures m_textures;

    typedef std::stack<std::shared_ptr<RenderTargetState> > RenderTargetStack;

    RenderTargetStack m_stack;
    Luminous::RenderContext & m_context;

    friend class RenderTargetObject;
  };

  //////////
  //////////

  class RenderTargetObject
  {
  public:
    RenderTargetObject(std::shared_ptr<RenderTargetManager::RenderTargetState> holder);
    ~RenderTargetObject();

  private:
    std::shared_ptr<RenderTargetManager::RenderTargetState> m_holder;
  };

}

#endif
