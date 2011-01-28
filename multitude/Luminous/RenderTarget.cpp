#include "RenderTarget.hpp"
#include "RenderContext.hpp"
#include "Error.hpp"

#include <Radiant/Trace.hpp>

#include <cassert>
#include <climits>

// In power-of-two
#define MINIMUM_FB_EXTENT 8

namespace Luminous
{

  static size_t nextHigherPowerOfTwo(size_t k)
  {
    if(k <= (1 << MINIMUM_FB_EXTENT))
      return MINIMUM_FB_EXTENT;

    k--;
    for(size_t i = 1; i < sizeof(size_t) * CHAR_BIT; i <<= 1)
      k = k | k >> 1;

    return k + 1;
  }

  //////////
  //////////

  RenderTargetManager::RenderTargetManager(Luminous::RenderContext & rc)
    : m_context(rc)
  {

  }

  RenderTargetObject RenderTargetManager::pushRenderTarget(Nimble::Vector2 reqSize, float scale)
  {
    Luminous::glErrorToString(__FILE__, __LINE__);

    //Radiant::info("RenderTargetManager::push");

    Nimble::Vector2i finalSize(int(reqSize.x * scale), int(reqSize.y * scale));
    size_t extent = size_t(finalSize.maximum());

    std::shared_ptr<RenderTargetState> holder = findAvailableTexture(extent);

    if(!holder.get())
      holder = allocateNewTexture(extent);

    assert(!holder->inUse);

    // Store viewport settings
    Nimble::Recti viewport(0, 0, holder->resource.texture().width(), holder->resource.texture().height());

    // Store matrices (these should modify the stack in RenderContext)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(viewport.low().x, viewport.width(), viewport.low().y, viewport.height(), -1, 1);
    Luminous::glErrorToString(__FILE__, __LINE__);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Make the render target active
    holder->resource.framebuffer().bind();
    Luminous::glErrorToString(__FILE__, __LINE__);

    glDrawBuffer(Luminous::COLOR0);
    Luminous::glErrorToString(__FILE__, __LINE__);

    // Apply viewport
    m_context.pushViewport(viewport);
    Luminous::glErrorToString(__FILE__, __LINE__);

    m_stack.push(holder);

    Luminous::glErrorToString(__FILE__, __LINE__);

    assert(holder->resource.framebuffer().check());

    return RenderTargetObject(holder);
  }

  Texture2D & RenderTargetManager::popRenderTarget(Luminous::RenderTargetObject & target)
  {
    //Radiant::info("RenderTargetManager::pop");

    if(m_stack.empty()) {
      Radiant::error("RenderTargetManager::popRenderTarget # stack is empty!");
    }

    // Get the active render target
    std::shared_ptr<RenderTargetState> active = m_stack.top();

    // Unbind and remove the active target from the stack
    active->resource.framebuffer().unbind();
    m_stack.pop();

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // If the stack is empty, restore rendering to back buffer otherwise
    // restore the target from the top of the stack
    if(m_stack.empty()) {
      glDrawBuffer(GL_BACK);
    } else {
      /*const RenderTargetState & state =*/ *m_stack.top().get();

      // Restore viewport and fb
      m_stack.top()->resource.framebuffer().bind();
      glDrawBuffer(Luminous::COLOR0);
    }

    // Restore viewport
    m_context.popViewport();

    Luminous::glErrorToString(__FILE__, __LINE__);

    // Return the texture that was attached to the previously active fb
    return active->resource.texture();
  }

  std::shared_ptr<RenderTargetManager::RenderTargetState> RenderTargetManager::allocateNewTexture(size_t extent)
  {
    Radiant::info("RenderTargetManager::allocateNewTexture # %ld next %ld",
                  extent, (int) (1 << nextHigherPowerOfTwo(extent)));

    // Use power-of-two textures
    extent = 1 << nextHigherPowerOfTwo(extent);

    std::shared_ptr<RenderTargetState> holder(new RenderTargetState());

    holder->resource.setSize(Nimble::Vector2i(extent, extent));
    holder->inUse = false;

    holder->resource.framebuffer().attachTexture2D(& holder->resource.texture(), Luminous::COLOR0, 0);

    // No side effects
    holder->resource.framebuffer().unbind();

    m_textures.insert(std::make_pair(extent, holder));

    Radiant::info("Created %ld texture", extent);

    return holder;
  }

  std::shared_ptr<RenderTargetManager::RenderTargetState> RenderTargetManager::findAvailableTexture(size_t extent)
  {
    for(Textures::iterator it = m_textures.lower_bound(extent); it != m_textures.end(); it++) {

      std::shared_ptr<RenderTargetState> holder = it->second;

      if(!holder->inUse)
        return holder;
    }

    std::shared_ptr<RenderTargetState> null;

    return null;
  }

  //////////
  //////////

  RenderTargetObject::RenderTargetObject(std::shared_ptr<RenderTargetManager::RenderTargetState> holder)
    : m_holder(holder)
  {
    assert(!m_holder->inUse);

    m_holder->inUse = true;
  }

  RenderTargetObject::~RenderTargetObject()
  {
    assert(m_holder->inUse);

    m_holder->inUse = false;
  }

}
