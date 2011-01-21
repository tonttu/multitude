/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

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
    FramebufferResource(Luminous::GLResources * r = 0);
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
