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
  class LUMINOUS_API Renderbuffer
  {
  public:
    Renderbuffer();
    ~Renderbuffer();

    /// Binds the buffer and creates it if necessary.
    void bind();
    /// Removes any GL_RENDERBUFFER bindings
    void unbind();

    /// Defines the format for the render buffer
    /// @param width width of the buffer
    /// @param height height of the buffer
    /// @param format pixel format for the render buffer
    void storageFormat(int width, int height, GLenum format);

  private:
    void create();

    GLuint m_bufferId;

    friend class Framebuffer;
  };

  /// Abstraction of an off-screen render target that can be used as a texture.
  class LUMINOUS_API Framebuffer
  {
  public:
    Framebuffer();
    ~Framebuffer();

    /// Binds the framebuffer
    void bind();
    /// Clears any GL_FRAMEBUFFER bindings
    void unbind();

    /// Checks the framebuffer for validity @return true if the framebuffer is
    /// valid and can be rendered into. Possible errors are sent to cerr.
    bool check();

    /// Attaches the framebuffer as 1D texture to the given attachment
    void attachTexture1D(Texture1D* texture,
             FramebufferAttachment attachment,
             int mipmapLevel = 0);
    /// Detaches the given 1D texture attachment
    void detachTexture1D(FramebufferAttachment attachment);

    /// Attaches the framebuffer as 2D texture to the given attachment
    void attachTexture2D(Texture2D* texture,
             FramebufferAttachment attachment,
             int mipmapLevel = 0);
    /// Detaches the given 2D texture attachment
    void detachTexture2D(FramebufferAttachment attachment);

    /// Attaches the framebuffer as 3D texture to the given attachment
    void attachTexture3D(Texture3D* texture,
             FramebufferAttachment attachment,
             int zSlice, int mipmapLevel = 0);
    /// Detaches the given 3D texture attachment
    void detachTexture3D(FramebufferAttachment attachment);

    /// Attaches the framebuffer as a single face of a cube texture to the given attachment
    void attachTextureCube(TextureCube* texture,
               FramebufferAttachment attachment,
               int face, int mipmapLevel = 0);
    /// Detaches the given cube texture attachment
    void detachTextureCube(FramebufferAttachment attachment, int face);

    /// Attaches the framebuffer as a renderbuffer attachment
    void attachRenderbuffer(Renderbuffer* renderbuffer,
                FramebufferAttachment attachment);
    /// Detaches the given renderbuffer attachment
    void detachRenderbuffer(FramebufferAttachment attachment);

    /// Deallocates the framebuffer object from the GPU
    void destroy();
  private:
    void create();
    GLuint m_bufferId;
  };

}

#endif
