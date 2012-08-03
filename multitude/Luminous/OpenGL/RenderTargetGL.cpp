#include "RenderTargetGL.hpp"

#include <Radiant/Mutex.hpp>

#include <QMap>

namespace Luminous
{

  RenderBufferGL::RenderBufferGL(StateGL &state)
    : ResourceHandleGL(state)
  {
    glGenRenderbuffers(1, &m_handle);
  }

  RenderBufferGL::~RenderBufferGL()
  {
    if(m_handle)
      glDeleteRenderbuffers(1, &m_handle);
  }


  void RenderBufferGL::bind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, m_handle);
  }

  void RenderBufferGL::unbind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }

  void RenderBufferGL::storageFormat(const QSize &size, GLenum format, int samples)
  {
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, size.width(), size.height());
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  RenderTargetGL::RenderTargetGL(StateGL &state)
    : ResourceHandleGL(state)
  {
    glGenFramebuffers(1, &m_handle);
  }

  RenderTargetGL::~RenderTargetGL()
  {
    glDeleteFramebuffers(1, &m_handle);
  }

  void RenderTargetGL::bind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
  }

  void RenderTargetGL::unbind()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void RenderTargetGL::attach(GLenum attachment, RenderBufferGL &renderBuffer)
  {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderBuffer.handle());
  }

  void RenderTargetGL::attach(GLenum attachment, TextureGL &texture)
  {
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.handle(), 0);
  }

  void RenderTargetGL::detach(GLenum attachment)
  {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0);
  }

  bool RenderTargetGL::check()
  {
    static QMap<GLuint, QString> errors;

    MULTI_ONCE_BEGIN {

      errors.insert(GL_FRAMEBUFFER_UNDEFINED, "GL_FRAMEBUFFER_UNDEFINED: target is the default framebuffer, but the default framebuffer does not exist.");
      errors.insert(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: any of the framebuffer attachment points are framebuffer incomplete.");
      errors.insert(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: the framebuffer does not have at least one image attached to it.");
      errors.insert(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAWBUFFERi.");
      errors.insert(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.");
      errors.insert(GL_FRAMEBUFFER_UNSUPPORTED, "GL_FRAMEBUFFER_UNSUPPORTED: the combination of internal formats of the attached images violates an implementation-dependent set of restrictions.");
      errors.insert(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES. Or  if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.");
      errors.insert(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.");

    } MULTI_ONCE_END

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if(status == GL_FRAMEBUFFER_COMPLETE)
      return true;

    Radiant::warning("RenderTargetGL::check # %s", errors.value(status).toUtf8().data());

    return false;
  }
}
