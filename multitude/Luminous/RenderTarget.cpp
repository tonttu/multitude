#include "RenderTarget.hpp"
#include "RenderContext.hpp"
#include "RenderManager.hpp"

#include <QSize>

namespace Luminous
{
  class RenderBuffer::D
  {
  public:
    D()
      : m_format(0)
      , m_samples(0)
    {}

    QSize m_size;
    GLenum m_format;
    int m_samples;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  RenderBuffer::RenderBuffer()
    : RenderResource(RenderResource::RenderBuffer)
    , m_d(new D())
  {
  }

  RenderBuffer::~RenderBuffer()
  {
    delete m_d;
  }

  RenderBuffer::RenderBuffer(RenderBuffer && rb)
    : RenderResource(std::move(rb))
    , m_d(rb.m_d)
  {
    rb.m_d = nullptr;
  }

  RenderBuffer & RenderBuffer::operator=(RenderBuffer && rb)
  {
    RenderResource::operator=(std::move(rb));
    std::swap(m_d, rb.m_d);
    return *this;
  }

  void RenderBuffer::storageFormat(const QSize &size, GLenum format, int samples)
  {
    m_d->m_size = size;
    m_d->m_format = format;
    m_d->m_samples = samples;
  }

  const QSize & RenderBuffer::size() const
  {
    return m_d->m_size;
  }

  GLenum RenderBuffer::format() const
  {
    return m_d->m_format;
  }

  int RenderBuffer::samples() const
  {
    return m_d->m_samples;
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTarget::D
  {
  public:
    RenderTargetType m_targetType;
    QSize m_size;
    QMap<GLenum, RenderResource::Id> m_textureAttachments;
    QMap<GLenum, RenderResource::Id> m_renderBufferAttachments;

    GLenum deduceBufferFormat(GLenum attachment) const
    {
      if(attachment == GL_DEPTH_ATTACHMENT)
        return GL_DEPTH_COMPONENT;
      else if(attachment == GL_STENCIL_ATTACHMENT)
        return GL_STENCIL_INDEX;
      else
        return GL_RGBA;
    }

  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////


  RenderTarget::RenderTarget(RenderTarget::RenderTargetType type)
    : RenderResource(RenderResource::FrameBuffer)
    , m_d(new D())
  {
    m_d->m_targetType = type;
  }

  RenderTarget::~RenderTarget()
  {
    delete m_d;
  }

  RenderTarget::RenderTarget(RenderTarget &&rt)
    : RenderResource(std::move(rt))
    , m_d(rt.m_d)
  {
    rt.m_d = nullptr;
  }

  RenderTarget & RenderTarget::operator=(RenderTarget && rt)
  {
    RenderResource::operator=(std::move(rt));
    std::swap(m_d, rt.m_d);
    return *this;
  }

  const QSize & RenderTarget::size() const
  {
    return m_d->m_size;
  }

  void RenderTarget::setSize(const QSize &size)
  {
    m_d->m_size= size;

    // Resize all attachments
    foreach(GLenum attachment, m_d->m_renderBufferAttachments.keys()) {
      auto rb = renderBuffer(attachment);
      rb->storageFormat(size, rb->format(), rb->samples());
    }

    foreach(GLenum attachment, m_d->m_textureAttachments.keys()) {
      auto t = texture(attachment);
      t->setData(size.width(), size.height(), t->dataFormat(), 0);
    }
  }

  void RenderTarget::attach(GLenum attachment, Luminous::RenderBuffer &buffer)
  {
    assert(m_d->m_targetType == RenderTarget::NORMAL);

    // If no format is specified, try to use something sensible based on attachment
    auto format = buffer.format();
    if(format == 0)
      format = m_d->deduceBufferFormat(attachment);

    buffer.storageFormat(size(), format, buffer.samples());

    m_d->m_renderBufferAttachments[attachment] = buffer.resourceId();
  }

  void RenderTarget::attach(GLenum attachment, Luminous::Texture &texture)
  {
    assert(m_d->m_targetType == RenderTarget::NORMAL);

    texture.setData(size().width(), size().height(), texture.dataFormat(), 0);

    m_d->m_textureAttachments[attachment] = texture.resourceId();
  }

  Luminous::Texture * RenderTarget::texture(GLenum attachment) const
  {
    if(m_d->m_textureAttachments.contains(attachment))
      return RenderManager::getResource<Luminous::Texture>(m_d->m_textureAttachments.value(attachment));

    return 0;
  }

  Luminous::RenderBuffer * RenderTarget::renderBuffer(GLenum attachment) const
  {
    if(m_d->m_renderBufferAttachments.contains(attachment))
      return RenderManager::getResource<Luminous::RenderBuffer>(m_d->m_renderBufferAttachments.value(attachment));

    return 0;
  }

  QList<GLenum> RenderTarget::textureAttachments() const
  {
    return m_d->m_textureAttachments.keys();
  }

  QList<GLenum> RenderTarget::renderBufferAttachments() const
  {
    return m_d->m_renderBufferAttachments.keys();
  }

  RenderTarget::RenderTargetType RenderTarget::targetType() const
  {
    return m_d->m_targetType;
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  RenderTargetGuard::RenderTargetGuard(RenderContext &r)
    : m_renderContext(r)
  {}

  RenderTargetGuard::~RenderTargetGuard()
  {
    /// @todo this should check that the current target is still valid (someone
    /// might manually pop it before)
    m_renderContext.popRenderTarget();
  }

}
