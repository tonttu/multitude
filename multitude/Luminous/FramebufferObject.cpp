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

#include <Luminous/FramebufferObject.hpp>
#include <iostream>
#include <cassert>

using namespace std;

namespace Luminous
{

  Renderbuffer::Renderbuffer()
      : m_bufferId((GLuint) -1)
  {
  }

  Renderbuffer::~Renderbuffer()
  {
    glDeleteRenderbuffersEXT(1, &m_bufferId);
  }

  void Renderbuffer::bind()
  {
    create();
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_bufferId);
  }

  void Renderbuffer::unbind()
  {
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
  }

  void Renderbuffer::storageFormat(int width, int height, GLenum format)
  {
    bind();
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, format, width, height);
  }


  void Renderbuffer::create()
  {
    if(m_bufferId == (GLuint) -1)
      glGenRenderbuffersEXT(1, &m_bufferId);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Framebuffer::Framebuffer()
      : m_bufferId((GLuint) -1)
  {
  }

  Framebuffer::~Framebuffer()
  {
    glDeleteFramebuffersEXT(1, &m_bufferId);
  }

  void Framebuffer::bind()
  {
    create();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_bufferId);
  }

  void Framebuffer::unbind()
  {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  }

  bool Framebuffer::check(void)
  {
    create();

    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

    switch(status)
    {
      case GL_FRAMEBUFFER_COMPLETE_EXT:
        return true;
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        cerr << "Error: Framebuffer object incomplete - attachment." << endl;
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        cerr << "Error: Framebuffer object incomplete - missing attachment." << endl;
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        cerr << "Error: Framebuffer object incomplete - dimensions." << endl;
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        cerr << "Error: Framebuffer object incomplete - formats." << endl;
        break;
     case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        cerr << "Error: Framebuffer object incomplete - draw buffer." << endl;
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        cerr << "Error: Framebuffer object incomplete - read buffer." << endl;
        break;
      case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        // Choose different format
        cerr << "Warning: Unsupported framebuffer object format. Try another format." << endl;
        break;
      default:
        // Programming error; will fail on all hardware
        assert(0);
        break;
    }

    return false;
  }

  void Framebuffer::attachTexture1D(Texture1D* texture, FramebufferAttachment attachment, int level)
  {
    bind();
    glFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, attachment,
                  GL_TEXTURE_1D, texture->id(), level);
  }

  void Framebuffer::detachTexture1D(FramebufferAttachment attachment)
  {
    bind();
    glFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_1D, 0, 0);
  }

  void Framebuffer::attachTexture2D(Texture2D* texture, FramebufferAttachment attachment, int level)
  {
    bind();
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D,
                  texture->id(), level);
  }

  void Framebuffer::detachTexture2D(FramebufferAttachment attachment)
  {
    bind();
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, 0, 0);
  }

  void Framebuffer::attachTexture3D(Texture3D* texture, FramebufferAttachment attachment, int zOffset, int level)
  {
    bind();
    glFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_3D,
                  texture->id(), level, zOffset);
  }

  void Framebuffer::detachTexture3D(FramebufferAttachment attachment)
  {
    bind();
    glFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_3D, 0, 0, 0);
  }

  void Framebuffer::attachTextureCube(TextureCube* texture, FramebufferAttachment attachment, int face, int level)
  {
    bind();
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment,
                  GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face,
                  texture->id(), level);
  }

  void Framebuffer::detachTextureCube(FramebufferAttachment attachment, int face)
  {
    bind();
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face, 0, 0);
  }

  void Framebuffer::attachRenderbuffer(Renderbuffer* renderbuffer, FramebufferAttachment attachment)
  {
    bind();
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, renderbuffer->m_bufferId);
  }

  void Framebuffer::detachRenderbuffer(FramebufferAttachment attachment)
  {
    bind();
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, 0);
  }

  void Framebuffer::create()
  {
    if(m_bufferId == (GLuint) -1)
      glGenFramebuffersEXT(1, &m_bufferId);
  }

}
