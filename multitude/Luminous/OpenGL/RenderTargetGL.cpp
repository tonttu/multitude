#include "RenderTargetGL.hpp"
#include "RenderDriverGL.hpp"

#include <Luminous/RenderTarget.hpp>

#include <Radiant/Mutex.hpp>

#include <QMap>

namespace Luminous
{

  RenderBufferGL::RenderBufferGL(StateGL &state)
    : ResourceHandleGL(state)
  {
    glGenRenderbuffers(1, &m_handle);
    GLERROR("RenderBufferGL::RenderBufferGL # glGenRenderbuffers");
  }

  RenderBufferGL::RenderBufferGL(RenderBufferGL && buffer)
    : ResourceHandleGL(std::move(buffer))
  {
  }

  RenderBufferGL::~RenderBufferGL()
  {
    if(m_handle)
      glDeleteRenderbuffers(1, &m_handle);
    GLERROR("RenderBufferGL::~RenderBufferGL # glDeleteRenderbuffers");
  }

  void RenderBufferGL::bind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, m_handle);
    GLERROR("RenderBufferGL::bind # glBindRenderbuffer");
  }

  void RenderBufferGL::unbind()
  {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GLERROR("RenderBufferGL::unbind # glBindRenderbuffer");
  }

  void RenderBufferGL::storageFormat(const QSize &size, GLenum format, int samples)
  {
    GLERROR("RenderBufferGL::storageFormat # zoo");

    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, size.width(), size.height());
    GLERROR("RenderBufferGL::storageFormat # glRenderbufferStorageMultisample");
  }

  void RenderBufferGL::sync(const RenderBuffer &buffer)
  {
    bind();
    storageFormat(buffer.size(), buffer.format(), buffer.samples());
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  RenderTargetGL::RenderTargetGL(StateGL &state)
    : ResourceHandleGL(state)
    , m_type(RenderTarget::INVALID)
  {
    glGenFramebuffers(1, &m_handle);
    GLERROR("RenderTargetGL::RenderTargetGL # glGenFramebuffers");
  }

  RenderTargetGL::RenderTargetGL(RenderTargetGL && target)
    : ResourceHandleGL(std::move(target))
    , m_type(std::move(target.m_type))
  {
  }

  RenderTargetGL::~RenderTargetGL()
  {
    glDeleteFramebuffers(1, &m_handle);
    GLERROR("RenderTargetGL::~RenderTargetGL # glDeleteFramebuffers");
  }

  void RenderTargetGL::bind()
  {
    assert(m_type != RenderTarget::INVALID);

    if(m_type == RenderTarget::WINDOW)
      unbind();
    else if(m_state.setFramebuffer(m_handle)) {
      glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
      GLERROR("RenderTargetGL::bind # glBindFramebuffer");
    }

    /// @todo should this be configurable?
    /// @todo this should be done by render context, not this class
    assert(m_size.isValid());
    glViewport(0, 0, m_size.width(), m_size.height());
    GLERROR("RenderTargetGL::bind # glViewport");
  }

  void RenderTargetGL::unbind()
  {
    if(m_state.setFramebuffer(0))
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLERROR("RenderTargetGL::unbind # glBindFramebuffer");
  }

  void RenderTargetGL::attach(GLenum attachment, RenderBufferGL &renderBuffer)
  {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderBuffer.handle());
    GLERROR("RenderTargetGL::attach # glFramebufferRenderbuffer");
  }

  void RenderTargetGL::attach(GLenum attachment, TextureGL &texture)
  {
    GLERROR("RenderTargetGL::attach # foo");

    texture.bind(0);
    GLERROR("RenderTargetGL::attach # mmoo");

    glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.handle(), 0);
    GLERROR("RenderTargetGL::attach # glFramebufferTexture");
  }

  void RenderTargetGL::detach(GLenum attachment)
  {
    /// @todo what about textures?
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0);
    GLERROR("RenderTargetGL::deattach # glFramebufferRenderbuffer");
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
    GLERROR("RenderTargetGL::check # glCheckFramebufferStatus");

    if(status == GL_FRAMEBUFFER_COMPLETE)
      return true;

    Radiant::warning("RenderTargetGL::check # %s", errors.value(status).toUtf8().data());

    return false;
  }

  void RenderTargetGL::sync(const RenderTarget &target)
  {
    m_type = target.targetType();
    m_size = target.size();

    bind();

    auto texAttachments = target.textureAttachments();
    auto bufAttachments = target.renderBufferAttachments();

    /// @todo should also detach removed attachments

    foreach(GLenum attachment, texAttachments)
      attach(attachment, m_state.driver().handle(*target.texture(attachment)));

    foreach(GLenum attachment, bufAttachments)
      attach(attachment, m_state.driver().handle(*target.renderBuffer(attachment)));

    check();
  }

}
