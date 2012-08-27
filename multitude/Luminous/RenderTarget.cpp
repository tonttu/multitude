#include "RenderTarget.hpp"
#include "RenderContext.hpp"
#include "RenderManager.hpp"

namespace Luminous
{
  class RenderBuffer::D
  {
  public:
    D()
      : m_format(0)
      , m_samples(0)
    {}

    Nimble::Size m_size;
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

  RenderBuffer::RenderBuffer(RenderBuffer & rb)
    : RenderResource(rb)
    , m_d(new RenderBuffer::D(*rb.m_d))
  {
  }

  RenderBuffer & RenderBuffer::operator=(RenderBuffer & rb)
  {
    if(this != &rb)
    {
      RenderResource::operator=(rb);
      assert(rb.m_d);
      *m_d = *rb.m_d;
    }
    return *this;
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

  void RenderBuffer::storageFormat(const Nimble::Size &size, GLenum format, int samples)
  {
    m_d->m_size = size;
    m_d->m_format = format;
    m_d->m_samples = samples;
  }

  const Nimble::Size &RenderBuffer::size() const
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
    Nimble::Size m_size;
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

  RenderTargetCopy::RenderTargetCopy(const RenderTarget & src, Type type)
    : m_src(src)
    , m_type(type)
  {
  }

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

  RenderTarget::RenderTarget(const RenderTargetCopy &rt)
    : RenderResource(RenderResource::FrameBuffer)
    , m_d(new D())
  {
    m_d->m_targetType = rt.m_src.m_d->m_targetType;
    m_d->m_size = rt.m_src.m_d->m_size;

    switch(rt.m_type) {
    case RenderTargetCopy::SHALLOW_COPY:
      m_d->m_textureAttachments = rt.m_src.m_d->m_textureAttachments;
      m_d->m_renderBufferAttachments = rt.m_src.m_d->m_renderBufferAttachments;
      break;
    case RenderTargetCopy::SHALLOW_COPY_NO_ATTACHMENTS:
      break;
    case RenderTargetCopy::DEEP_COPY:
      /// @todo implement
      assert(0);
      break;
    };
  }

  RenderTargetCopy RenderTarget::shallowCopy() const
  {
    return RenderTargetCopy(*this, RenderTargetCopy::SHALLOW_COPY);
  }

  RenderTargetCopy RenderTarget::deepCopy() const
  {
    return RenderTargetCopy(*this, RenderTargetCopy::DEEP_COPY);
  }

  RenderTargetCopy RenderTarget::shallowCopyNoAttachments() const
  {
    return RenderTargetCopy(*this, RenderTargetCopy::SHALLOW_COPY_NO_ATTACHMENTS);
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

  const Nimble::Size &RenderTarget::size() const
  {
    return m_d->m_size;
  }

  void RenderTarget::setSize(const Nimble::Size &size)
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
