#ifndef LUMINOUS_FRAMEBUFFER_RESOURCE_HPP
#define LUMINOUS_FRAMEBUFFER_RESOURCE_HPP

#include <Luminous/FramebufferObject.hpp>
#include <Luminous/Export.hpp>

namespace Luminous
{

  /// A framebuffer object and texture pair. Useful for implementing render-to-texture.
  class LUMINOUS_API FramebufferResource : public Luminous::GLResource
  {
  public:
    /// Constructs a new framebuffer resource
    FramebufferResource(Luminous::RenderContext * r = 0);
    virtual ~FramebufferResource();

    /// Changes the size of this of the texture. A valid OpenGL context must be active.
    void setSize(Nimble::Vector2i size);

    /// Returns the framebuffer object for this resource
    inline Luminous::Framebuffer & framebuffer() { return m_fbo; }
    /// Returns the texture object for this resource
    inline Luminous::Texture2D & texture() { return m_tex; }

  private:
    Luminous::Framebuffer   m_fbo;
    Luminous::Texture2D     m_tex;
  };

}

#endif
