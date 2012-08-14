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
#include <Luminous/OpenGL/Error.hpp>

#include <iostream>
#include <cassert>

using namespace std;

namespace Luminous
{

  Renderbuffer::Renderbuffer(Luminous::RenderContext * res)
    : GLResource(res),
    m_bufferId((GLuint) -1)
  {
  }

  Renderbuffer::~Renderbuffer()
  {
    glDeleteRenderbuffers(1, &m_bufferId);
  }

  void Renderbuffer::bind()
  {
    create();
    glBindRenderbuffer(GL_RENDERBUFFER, m_bufferId);
  }

  void Renderbuffer::unbind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }

  void Renderbuffer::storageFormat(int width, int height, GLenum format)
  {
    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
  }


  void Renderbuffer::create()
  {
    if(m_bufferId == (GLuint) -1)
      glGenRenderbuffers(1, &m_bufferId);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Framebuffer::Framebuffer(Luminous::RenderContext * res)
    : GLResource(res),
    m_bufferId((GLuint) -1)
  {
  }

  Framebuffer::~Framebuffer()
  {
    glDeleteFramebuffers(1, &m_bufferId);
  }

  void Framebuffer::bind()
  {
    create();
    glBindFramebuffer(GL_FRAMEBUFFER, m_bufferId);
    Luminous::glErrorToString(__FILE__, __LINE__);
  }

  void Framebuffer::unbind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  bool Framebuffer::check(void)
  {
    static QMap<GLuint, QString> errors;

    MULTI_ONCE {

        errors.insert(GL_FRAMEBUFFER_UNDEFINED, "GL_FRAMEBUFFER_UNDEFINED: target is the default framebuffer, but the default framebuffer does not exist.");
        errors.insert(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: any of the framebuffer attachment points are framebuffer incomplete.");
        errors.insert(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: the framebuffer does not have at least one image attached to it.");
        errors.insert(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAWBUFFERi.");
        errors.insert(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.");
        errors.insert(GL_FRAMEBUFFER_UNSUPPORTED, "GL_FRAMEBUFFER_UNSUPPORTED: the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.");
        errors.insert(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES. Or  if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.");
        errors.insert(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.");

    }

    create();

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if(status == GL_FRAMEBUFFER_COMPLETE)
        return true;

    Radiant::error("Framebuffer::check # %s", errors.value(status).toUtf8().data());
    return false;
  }

  void Framebuffer::attachTexture2D(Texture2D* texture, GLenum attachment, int level)
  {
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
                  texture->id(), level);
  }

  void Framebuffer::detachTexture2D(GLenum attachment)
  {
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, 0, 0);
  }

#ifndef LUMINOUS_OPENGLES

  void Framebuffer::attachTexture1D(Texture1D* texture, GLenum attachment, int level)
  {
    bind();
    glFramebufferTexture1D(GL_FRAMEBUFFER, attachment,
                  GL_TEXTURE_1D, texture->id(), level);
  }

  void Framebuffer::detachTexture1D(GLenum attachment)
  {
    bind();
    glFramebufferTexture1D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_1D, 0, 0);
  }

  void Framebuffer::attachTexture3D(Texture3D* texture, GLenum attachment, int zOffset, int level)
  {
    bind();
    glFramebufferTexture3D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_3D,
                  texture->id(), level, zOffset);
  }

  void Framebuffer::detachTexture3D(GLenum attachment)
  {
    bind();
    glFramebufferTexture3D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_3D, 0, 0, 0);
  }

  void Framebuffer::attachTextureCube(TextureCube* texture, GLenum attachment, int face, int level)
  {
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
                  GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                  texture->id(), level);
  }

  void Framebuffer::detachTextureCube(GLenum attachment, int face)
  {
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, 0);
  }

#endif // LUMINOUS_OPENGLES

  void Framebuffer::attachRenderbuffer(Renderbuffer* renderbuffer, GLenum attachment)
  {
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderbuffer->m_bufferId);
  }

  void Framebuffer::detachRenderbuffer(GLenum attachment)
  {
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0);
  }

  void Framebuffer::create()
  {
    if(m_bufferId == (GLuint) -1)
      glGenFramebuffers(1, &m_bufferId);
    Luminous::glErrorToString(__FILE__, __LINE__);
  }

  void Framebuffer::destroy()
  {
    if(m_bufferId != (GLuint)-1) {
      glDeleteFramebuffers(1, &m_bufferId);
      m_bufferId = (GLuint)-1;
    }
  }

}
