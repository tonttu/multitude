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
    if(m_d->m_size == size &&
       m_d->m_format == format &&
       m_d->m_samples == samples)
      return;
    m_d->m_size = size;
    m_d->m_format = format;
    m_d->m_samples = samples;
    invalidate();
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

    std::vector<std::unique_ptr<Luminous::Texture> > m_ownedTextureAttachments;
    std::vector<std::unique_ptr<Luminous::RenderBuffer> > m_ownedRenderBufferAttachments;

    GLenum deduceBufferFormat(GLenum attachment) const
    {
      if(attachment == GL_DEPTH_ATTACHMENT)
        return GL_DEPTH_COMPONENT;
      else if(attachment == GL_STENCIL_ATTACHMENT)
        return GL_STENCIL_INDEX;
      else
        return GL_RGBA;
    }

    void attach(GLenum attachment, Luminous::Texture &texture)
    {
      assert(m_targetType == RenderTarget::NORMAL);

      texture.setData(m_size.width(), m_size.height(), texture.dataFormat(), 0);

      m_textureAttachments[attachment] = texture.resourceId();
    }

    void attach(GLenum attachment, Luminous::RenderBuffer &buffer)
    {
      assert(m_targetType == RenderTarget::NORMAL);

      // If no format is specified, try to use something sensible based on attachment
      auto format = buffer.format();
      if(format == 0)
        format = deduceBufferFormat(attachment);

      buffer.storageFormat(m_size, format, buffer.samples());

      m_renderBufferAttachments[attachment] = buffer.resourceId();
    }

    Luminous::Texture & createTextureAttachment(GLenum attachment, const Luminous::PixelFormat & format)
    {
      auto tex = std::unique_ptr<Luminous::Texture>(new Luminous::Texture());
      tex->setData(m_size.width(), m_size.height(), format, 0);

      attach(attachment, *tex);

      auto result = tex.get();

      m_ownedTextureAttachments.push_back(std::move(tex));

      return *result;
    }

    Luminous::RenderBuffer & createRenderBufferAttachment(GLenum attachment, GLenum storageFormat)
    {
      auto buf = std::unique_ptr<Luminous::RenderBuffer>(new Luminous::RenderBuffer());

      /// @todo what samples?
      buf->storageFormat(m_size, storageFormat, 0);

      attach(attachment, *buf);

      auto result = buf.get();

      m_ownedRenderBufferAttachments.push_back(std::move(buf));

      return *result;
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

  RenderTarget::RenderTarget(const RenderTargetCopy &rt)
    : RenderResource(RenderResource::FrameBuffer)
    , m_d(rt.m_d)
  {
  }

  RenderTarget::RenderTargetCopy RenderTarget::shallowCopyNoAttachments() const
  {
    auto d = new RenderTarget::D();

    d->m_targetType = m_d->m_targetType;
    d->m_size = m_d->m_size;

    return RenderTargetCopy(d);
  }

  RenderTarget::RenderTargetCopy RenderTarget::shallowCopy() const
  {
    // First, make a shallow copy with no attachments
    auto d = shallowCopyNoAttachments();

    // Make a shallow copy of the attachments (copy resource Ids)
    d.m_d->m_textureAttachments = m_d->m_textureAttachments;
    d.m_d->m_renderBufferAttachments = m_d->m_renderBufferAttachments;

    return RenderTargetCopy(d);
  }

  RenderTarget::RenderTargetCopy RenderTarget::deepCopy() const
  {
    // First, make a shallow copy with no attachments
    auto d = shallowCopyNoAttachments();

    // Make a deep copy of attachments and attach the copies
    for(auto i = m_d->m_textureAttachments.begin(); i != m_d->m_textureAttachments.end(); ++i) {

      GLenum attachment = i.key();
      RenderResource::Id resourceId = i.value();

      Luminous::Texture * tex = RenderManager::getResource<Luminous::Texture>(resourceId);

      d.m_d->createTextureAttachment(attachment, tex->dataFormat());
    }

    for(auto i = m_d->m_renderBufferAttachments.begin(); i != m_d->m_renderBufferAttachments.end(); ++i) {

      GLenum attachment = i.key();
      RenderResource::Id resourceId = i.value();

      Luminous::RenderBuffer * buf = RenderManager::getResource<Luminous::RenderBuffer>(resourceId);

      d.m_d->createRenderBufferAttachment(attachment, buf->format());
    }

    return RenderTargetCopy(d);
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
    m_d->attach(attachment, buffer);
  }

  void RenderTarget::attach(GLenum attachment, Luminous::Texture &texture)
  {
    m_d->attach(attachment, texture);
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

  Texture & RenderTarget::createTextureAttachment(GLenum attachment, const Luminous::PixelFormat & format)
  {
    return m_d->createTextureAttachment(attachment, format);
  }

  RenderBuffer & RenderTarget::createRenderBufferAttachment(GLenum attachment, GLenum storageFormat)
  {
    return m_d->createRenderBufferAttachment(attachment, storageFormat);
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
