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
      : m_format(GL_RGB)
    {}

    QSize m_size;
    GLenum m_format;
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

  void RenderBuffer::storageFormat(const QSize &size, GLenum format)
  {
    m_d->m_size = size;
    m_d->m_format = format;
  }

  const QSize & RenderBuffer::size() const
  {
    return m_d->m_size;
  }

  GLenum RenderBuffer::format() const
  {
    return m_d->m_format;
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTarget::D
  {
  public:
    QSize m_size;
    QMap<GLenum, RenderResource::Id> m_textureAttachments;
    QMap<GLenum, RenderResource::Id> m_renderBufferAttachments;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////


  RenderTarget::RenderTarget()
    : RenderResource(RenderResource::FrameBuffer)
    , m_d(new D())
  {}

  RenderTarget::~RenderTarget()
  {
    delete m_d;
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
      rb->storageFormat(size, rb->format());
    }

    foreach(GLenum attachment, m_d->m_textureAttachments.keys()) {
      auto t = texture(attachment);
      t->setData(size.width(), size.height(), t->dataFormat(), 0);
    }
  }

  void RenderTarget::attach(GLenum attachment, Luminous::RenderBuffer &buffer)
  {
    buffer.storageFormat(size(), buffer.format());

    m_d->m_renderBufferAttachments[attachment] = buffer.resourceId();
  }

  void RenderTarget::attach(GLenum attachment, Luminous::Texture &texture)
  {
    texture.setData(size().width(), size().height(), texture.dataFormat(), 0);

    m_d->m_textureAttachments[attachment] = texture.resourceId();
  }

  Luminous::Texture * RenderTarget::texture(GLenum attachment)
  {
    if(m_d->m_textureAttachments.contains(attachment))
      return RenderManager::getResource<Luminous::Texture>(m_d->m_textureAttachments.value(attachment));

    return 0;
  }

  Luminous::RenderBuffer * RenderTarget::renderBuffer(GLenum attachment)
  {
    if(m_d->m_renderBufferAttachments.contains(attachment))
      return RenderManager::getResource<Luminous::RenderBuffer>(m_d->m_renderBufferAttachments.value(attachment));

    return 0;
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  RenderTargetGuard::RenderTargetGuard(RenderContext &r)
    : m_renderContext(r)
  {}

  RenderTargetGuard::~RenderTargetGuard()
  {
    m_renderContext.popRenderTarget();
  }

}
