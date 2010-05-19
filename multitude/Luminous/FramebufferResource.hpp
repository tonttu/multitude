#ifndef LUMINOUS_FRAMEBUFFER_RESOURCE_HPP
#define LUMINOUS_FRAMEBUFFER_RESOURCE_HPP

#include <Luminous/FramebufferObject.hpp>

namespace Luminous
{

  class LUMINOUS_API FramebufferResource : public Luminous::GLResource
  {
  public:
    FramebufferResource(Luminous::GLResources * r = 0);
    virtual ~FramebufferResource();

    /// Changes the size of this of the texture. A valid OpenGL context must be active.
    void setSize(Nimble::Vector2i size);

    /// Returns the framebuffer object for this resource
    Luminous::Framebuffer & framebuffer() { return m_fbo; }
    /// Returns the texture object for this resource
    Luminous::Texture2D & texture() { return m_tex; }

    void setGeneration(size_t g) { m_generationCounter = g; }
    size_t generation() const { return m_generationCounter; }

  private:
    Luminous::Framebuffer   m_fbo;
    Luminous::Texture2D     m_tex;
    size_t m_generationCounter;
  };

}

#endif
