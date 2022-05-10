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
#include "RenderManager.hpp"

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
    if (m_generation == buffer.generation()) {
      touch();
      return;
    }
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
    , m_size(std::move(target.m_size))
    , m_generation(std::move(target.m_generation))
    , m_textureAttachments(std::move(target.m_textureAttachments))
    , m_renderBufferAttachments(std::move(target.m_renderBufferAttachments))
    , m_dirty(std::move(target.m_dirty))
  {
  }

  FrameBufferGL::~FrameBufferGL()
  {
    for (TextureGL * texGl: m_textureAttachments)
      texGl->unref();
    for (RenderBufferGL * renderTargetGl: m_renderBufferAttachments)
      renderTargetGl->unref();
    m_state.opengl().glDeleteFramebuffers(1, &m_handle);
    GLERROR("FrameBufferGL::~FrameBufferGL # glDeleteFramebuffers");
  }

  void FrameBufferGL::bind()
  {
    syncImpl();
    bindImpl();
  }

  void FrameBufferGL::bindImpl()
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
    m_state.opengl().glFramebufferRenderbuffer(bindTarget(m_bind), attachment, GL_RENDERBUFFER, renderBuffer.handle());
    GLERROR("FrameBufferGL::attach # glFramebufferRenderbuffer");
  }

  void FrameBufferGL::attach(GLenum attachment, TextureGL &texture)
  {
    texture.bind(0);

    if (texture.samples() > 0) {
      m_state.opengl().glFramebufferTexture2D(bindTarget(m_bind), attachment, GL_TEXTURE_2D_MULTISAMPLE, texture.handle(), 0);
    } else {
      m_state.opengl().glFramebufferTexture2D(bindTarget(m_bind), attachment, GL_TEXTURE_2D, texture.handle(), 0);
    }
    GLERROR("FrameBufferGL::attach # glFramebufferTexture");
  }

  void FrameBufferGL::detach(GLenum attachment)
  {
    /// @todo what about textures?
    m_state.opengl().glFramebufferRenderbuffer(bindTarget(m_bind), attachment, GL_RENDERBUFFER, 0);
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

    GLenum status = m_state.opengl().glCheckFramebufferStatus(bindTarget(m_bind));
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
    if (target.generation() == m_generation)
      return;

    m_generation = target.generation();
    m_dirty = true;
    m_type = target.targetType();
    m_bind = target.targetBind();
    m_size = target.size();

    // Step m_textureAttachments and textures in sync, both are ordered by
    // the attachment type
    {
      const QMap<GLenum, RenderResource::Id> & textures = target.textureAttachments();
      auto git = m_textureAttachments.begin();

      for (auto it = textures.begin(); it != textures.end(); ++it) {
        Texture * tex = RenderManager::getResource<Texture>(it.value());
        if (!tex)
          continue;

        auto & texGl = m_state.driver().handle(*tex);
        texGl.upload(*tex, 0, TextureGL::UPLOAD_SYNC);

        while (git != m_textureAttachments.end() && git.key() < it.key()) {
          git.value()->unref();
          git = m_textureAttachments.erase(git);
        }

        if (git != m_textureAttachments.end() && git.key() == it.key()) {
          if (&texGl != git.value()) {
            git.value()->unref();
            texGl.ref();
            *git = &texGl;
          }
          ++git;
          continue;
        }

        texGl.ref();
        git = m_textureAttachments.insert(it.key(), &texGl);
        ++git;
      }

      while (git != m_textureAttachments.end()) {
        git.value()->unref();
        git = m_textureAttachments.erase(git);
      }
    }

    // Same for m_renderBufferAttachments
    {
      const QMap<GLenum, RenderResource::Id> & renderBuffers = target.renderBufferAttachments();
      auto git = m_renderBufferAttachments.begin();

      for (auto it = renderBuffers.begin(); it != renderBuffers.end(); ++it) {
        RenderBuffer * renderBuffer = RenderManager::getResource<RenderBuffer>(it.value());
        if (!renderBuffer)
          continue;

        auto & renderBufferGl = m_state.driver().handle(*renderBuffer);
        renderBufferGl.sync(*renderBuffer);

        while (git != m_renderBufferAttachments.end() && git.key() < it.key()) {
          git.value()->unref();
          git = m_renderBufferAttachments.erase(git);
        }

        if (git != m_renderBufferAttachments.end() && git.key() == it.key()) {
          if (&renderBufferGl != git.value()) {
            git.value()->unref();
            renderBufferGl.ref();
            *git = &renderBufferGl;
          }
          ++git;
          continue;
        }

        renderBufferGl.ref();
        git = m_renderBufferAttachments.insert(it.key(), &renderBufferGl);
        ++git;
      }

      while (git != m_renderBufferAttachments.end()) {
        git.value()->unref();
        git = m_renderBufferAttachments.erase(git);
      }
    }
  }

  void FrameBufferGL::syncImpl()
  {
    if (!m_dirty)
      return;
    m_dirty = false;

    bindImpl();

    /// @todo should also detach removed attachments

    for (auto it = m_textureAttachments.begin(); it != m_textureAttachments.end(); ++it)
      attach(it.key(), *it.value());

    for (auto it = m_renderBufferAttachments.begin(); it != m_renderBufferAttachments.end(); ++it)
      attach(it.key(), *it.value());

    check();
  }
}
