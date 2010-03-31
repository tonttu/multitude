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

#ifndef LUMINOUS_FRAMEBUFFEROBJECT_HPP
#define LUMINOUS_FRAMEBUFFEROBJECT_HPP

#include <Luminous/Texture.hpp>

/// @todo Rename file to FrameBuffer.hpp & RenderBuffer.hpp

namespace Luminous
{

  /// @todo remove, and use the GL constants directly
  enum FramebufferAttachment
  {
    COLOR0  = GL_COLOR_ATTACHMENT0_EXT,
    COLOR1  = GL_COLOR_ATTACHMENT1_EXT,
    COLOR2  = GL_COLOR_ATTACHMENT2_EXT,
    COLOR3  = GL_COLOR_ATTACHMENT3_EXT,
    COLOR4  = GL_COLOR_ATTACHMENT4_EXT,
    COLOR5  = GL_COLOR_ATTACHMENT5_EXT,
    COLOR6  = GL_COLOR_ATTACHMENT6_EXT,
    COLOR7  = GL_COLOR_ATTACHMENT7_EXT,
    COLOR8  = GL_COLOR_ATTACHMENT8_EXT,
    COLOR9  = GL_COLOR_ATTACHMENT9_EXT,
    COLOR10 = GL_COLOR_ATTACHMENT10_EXT,
    COLOR11 = GL_COLOR_ATTACHMENT11_EXT,
    COLOR12 = GL_COLOR_ATTACHMENT12_EXT,
    COLOR13 = GL_COLOR_ATTACHMENT13_EXT,
    COLOR14 = GL_COLOR_ATTACHMENT14_EXT,
    COLOR15 = GL_COLOR_ATTACHMENT15_EXT,
    DEPTH   = GL_DEPTH_ATTACHMENT_EXT,
    STENCIL = GL_STENCIL_ATTACHMENT_EXT
  };

  /// An abstraction of an off-screen render target.
  /// @todo Doc
  class LUMINOUS_API Renderbuffer
  {
  public:
    Renderbuffer();
    ~Renderbuffer();

    void bind();
    void unbind();

    void storageFormat(int width, int height, GLenum format);

  private:
    void create();

    GLuint m_bufferId;

    friend class Framebuffer;
  };

  /// Abstraction of an off-screen render target that can be used as a texture.
  /// @todo Doc
  class LUMINOUS_API Framebuffer
  {
  public:
    Framebuffer();
    ~Framebuffer();

    void bind();
    void unbind();

    bool check();

    void attachTexture1D(Texture1D* texture,
             FramebufferAttachment attachment,
             int mipmapLevel = 0);
    void detachTexture1D(FramebufferAttachment attachment);

    void attachTexture2D(Texture2D* texture,
             FramebufferAttachment attachment,
             int mipmapLevel = 0);
    void detachTexture2D(FramebufferAttachment attachment);

    void attachTexture3D(Texture3D* texture,
             FramebufferAttachment attachment,
             int zSlice, int mipmapLevel = 0);
    void detachTexture3D(FramebufferAttachment attachment);

    void attachTextureCube(TextureCube* texture,
               FramebufferAttachment attachment,
               int face, int mipmapLevel = 0);
    void detachTextureCube(FramebufferAttachment attachment, int face);

    void attachRenderbuffer(Renderbuffer* renderbuffer,
                FramebufferAttachment attachment);
    void detachRenderbuffer(FramebufferAttachment attachment);

  private:
    void create();
    GLuint m_bufferId;
  };

}

#endif
