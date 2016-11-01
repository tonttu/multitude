/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "FrameBufferGL.hpp"
#include "RenderDriverGL.hpp"

#include <Luminous/FrameBuffer.hpp>

#include <Radiant/Mutex.hpp>

#include <QMap>

namespace Luminous
{

  RenderBufferGL::RenderBufferGL(StateGL &state)
    : ResourceHandleGL(state)
    , m_generation(0)
  {
    m_state.opengl().glGenRenderbuffers(1, &m_handle);
    GLERROR("RenderBufferGL::RenderBufferGL # glGenRenderbuffers");
  }

  RenderBufferGL::RenderBufferGL(RenderBufferGL && buffer)
    : ResourceHandleGL(std::move(buffer))
    , m_generation(buffer.m_generation)
  {
  }

  RenderBufferGL::~RenderBufferGL()
  {
    if(m_handle)
      m_state.opengl().glDeleteRenderbuffers(1, &m_handle);
    GLERROR("RenderBufferGL::~RenderBufferGL # glDeleteRenderbuffers");
  }

  void RenderBufferGL::bind()
  {
    m_state.opengl().glBindRenderbuffer(GL_RENDERBUFFER, m_handle);
    GLERROR("RenderBufferGL::bind # glBindRenderbuffer");

    touch();
  }

  void RenderBufferGL::unbind()
  {
    m_state.opengl().glBindRenderbuffer(GL_RENDERBUFFER, 0);
    GLERROR("RenderBufferGL::unbind # glBindRenderbuffer");
  }

  void RenderBufferGL::setStorageFormat(const RenderBuffer & buffer)
  {
    GLERROR("RenderBufferGL::storageFormat # zoo");
    touch();

    if(m_generation != buffer.generation()) {
      m_generation = buffer.generation();

      m_state.opengl().glRenderbufferStorageMultisample(GL_RENDERBUFFER, buffer.samples(), buffer.format(), buffer.size().width(), buffer.size().height());
      GLERROR("RenderBufferGL::storageFormat # glRenderbufferStorageMultisample");
    }
  }

  void RenderBufferGL::sync(const RenderBuffer & buffer)
  {
    bind();
    setStorageFormat(buffer);
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  namespace
  {
    GLenum bindTarget(FrameBuffer::FrameBufferBind target)
    {
      switch(target)
      {
      case FrameBuffer::BIND_DEFAULT:
        return GL_FRAMEBUFFER;
      case FrameBuffer::BIND_DRAW:
        return GL_DRAW_FRAMEBUFFER;
      case FrameBuffer::BIND_READ:
        return GL_READ_FRAMEBUFFER;
      default:
        assert(false);
        return GL_FRAMEBUFFER;
      }
    }
  }

  FrameBufferGL::FrameBufferGL(StateGL &state)
    : ResourceHandleGL(state)
    , m_type(FrameBuffer::INVALID)
    , m_bind(FrameBuffer::BIND_DEFAULT)
  {
    m_state.opengl().glGenFramebuffers(1, &m_handle);
    GLERROR("FrameBufferGL::FrameBufferGL # glGenFramebuffers");
  }

  FrameBufferGL::FrameBufferGL(FrameBufferGL && target)
    : ResourceHandleGL(std::move(target))
    , m_type(std::move(target.m_type))
    , m_bind(std::move(target.m_bind))
  {
  }

  FrameBufferGL::~FrameBufferGL()
  {
    m_state.opengl().glDeleteFramebuffers(1, &m_handle);
    GLERROR("FrameBufferGL::~FrameBufferGL # glDeleteFramebuffers");
  }

  void FrameBufferGL::bind()
  {
    assert(m_type != FrameBuffer::INVALID);

    if(m_type == FrameBuffer::WINDOW)
      unbind();
    else if(m_state.setFramebuffer(bindTarget(m_bind), m_handle)) {
      m_state.opengl().glBindFramebuffer(bindTarget(m_bind), m_handle);
      GLERROR("FrameBufferGL::bind # glBindFramebuffer");
    }

    touch();
  }

  void FrameBufferGL::unbind()
  {
    if(m_state.setFramebuffer(bindTarget(m_bind), 0))
      m_state.opengl().glBindFramebuffer(bindTarget(m_bind), 0);
    GLERROR("FrameBufferGL::unbind # glBindFramebuffer");
  }

  void FrameBufferGL::attach(GLenum attachment, RenderBufferGL &renderBuffer)
  {
    m_state.opengl().glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderBuffer.handle());
    GLERROR("FrameBufferGL::attach # glFramebufferRenderbuffer");
  }

  void FrameBufferGL::attach(GLenum attachment, TextureGL &texture)
  {
    GLERROR("FrameBufferGL::attach # foo");

    texture.bind(0);
    GLERROR("FrameBufferGL::attach # mmoo");

    m_state.opengl().glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.handle(), 0);
    GLERROR("FrameBufferGL::attach # glFramebufferTexture");
  }

  void FrameBufferGL::detach(GLenum attachment)
  {
    /// @todo what about textures?
    m_state.opengl().glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0);
    GLERROR("FrameBufferGL::deattach # glFramebufferRenderbuffer");
  }

  bool FrameBufferGL::check()
  {
    // Only do actual checking in debug mode since this apparently slows things down quite a bit (10% in twinkle)
#if RADIANT_DEBUG
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

    GLenum status = m_state.opengl().glCheckFramebufferStatus(GL_FRAMEBUFFER);
    GLERROR("FrameBufferGL::check # glCheckFramebufferStatus");

    if(status == GL_FRAMEBUFFER_COMPLETE)
      return true;

    Radiant::warning("FrameBufferGL::check # %s", errors.value(status).toUtf8().data());

    return false;
#else
    return true;
#endif
  }

  void FrameBufferGL::sync(const FrameBuffer &target)
  {
    m_type = target.targetType();
    m_bind = target.targetBind();
    m_size = target.size();

    bind();

    auto texAttachments = target.textureAttachments();
    auto bufAttachments = target.renderBufferAttachments();

    /// @todo should also detach removed attachments

    for(GLenum attachment : texAttachments)
      attach(attachment, m_state.driver().handle(*target.texture(attachment)));

    for(GLenum attachment : bufAttachments)
      attach(attachment, m_state.driver().handle(*target.renderBuffer(attachment)));

    check();
  }

}
