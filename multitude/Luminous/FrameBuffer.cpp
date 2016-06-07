/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "FrameBuffer.hpp"
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

  void RenderBuffer::setStorageFormat(const Nimble::Size &size, GLenum format, int samples)
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

  class FrameBuffer::D
  {
  public:
    FrameBufferType m_targetType;
    FrameBufferBind m_targetBind;
    Nimble::Size m_size;
    unsigned m_samples;
    QMap<GLenum, RenderResource::Id> m_textureAttachments;
    QMap<GLenum, RenderResource::Id> m_renderBufferAttachments;

    std::vector<std::unique_ptr<Luminous::Texture> > m_ownedTextureAttachments;
    std::vector<std::unique_ptr<Luminous::RenderBuffer> > m_ownedRenderBufferAttachments;

    D()
      : m_targetBind(FrameBuffer::BIND_DEFAULT)
      , m_samples(0)
    {}

    GLenum deduceBufferFormat(GLenum attachment) const
    {
      if(attachment == GL_DEPTH_ATTACHMENT)
        return GL_DEPTH_COMPONENT;
      else if(attachment == GL_STENCIL_ATTACHMENT)
        return GL_STENCIL_INDEX;
      else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
        return GL_DEPTH24_STENCIL8;
      else
        return GL_RGBA;
    }

    void attach(GLenum attachment, Luminous::Texture &texture)
    {
      assert(m_targetType == FrameBuffer::NORMAL);

      texture.setData(m_size.width(), m_size.height(), texture.dataFormat(), 0);

      m_textureAttachments[attachment] = texture.resourceId();
    }

    void attach(GLenum attachment, Luminous::RenderBuffer &buffer)
    {
      assert(m_targetType == FrameBuffer::NORMAL);

      // If no format is specified, try to use something sensible based on attachment
      auto format = buffer.format();
      if(format == 0)
        format = deduceBufferFormat(attachment);

      buffer.setStorageFormat(m_size, format, m_samples);

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

      buf->setStorageFormat(m_size, storageFormat, m_samples);

      attach(attachment, *buf);

      auto result = buf.get();

      m_ownedRenderBufferAttachments.push_back(std::move(buf));

      return *result;
    }

  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  FrameBuffer::FrameBuffer(FrameBuffer::FrameBufferType type)
    : RenderResource(RenderResource::FrameBuffer)
    , m_d(new D())
  {
    m_d->m_targetType = type;
  }

  FrameBuffer::~FrameBuffer()
  {
    delete m_d;
  }

  FrameBuffer::FrameBuffer(const FrameBufferCopy &rt)
    : RenderResource(RenderResource::FrameBuffer)
    , m_d(rt.m_d)
  {
  }

  FrameBuffer::FrameBufferCopy FrameBuffer::shallowCopyNoAttachments() const
  {
    auto d = new FrameBuffer::D();

    d->m_targetType = m_d->m_targetType;
    d->m_size = m_d->m_size;
    d->m_samples = m_d->m_samples;

    return FrameBufferCopy(d);
  }

  FrameBuffer::FrameBufferCopy FrameBuffer::shallowCopy() const
  {
    // First, make a shallow copy with no attachments
    auto d = shallowCopyNoAttachments();

    // Make a shallow copy of the attachments (copy resource Ids)
    d.m_d->m_textureAttachments = m_d->m_textureAttachments;
    d.m_d->m_renderBufferAttachments = m_d->m_renderBufferAttachments;

    return FrameBufferCopy(d);
  }

  FrameBuffer::FrameBufferCopy FrameBuffer::deepCopy() const
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

    return FrameBufferCopy(d);
  }

  FrameBuffer::FrameBuffer(FrameBuffer &&rt)
    : RenderResource(std::move(rt))
    , m_d(rt.m_d)
  {
    rt.m_d = nullptr;
  }

  FrameBuffer & FrameBuffer::operator=(FrameBuffer && rt)
  {
    RenderResource::operator=(std::move(rt));
    std::swap(m_d, rt.m_d);
    return *this;
  }

  const Nimble::Size &FrameBuffer::size() const
  {
    return m_d->m_size;
  }

  void FrameBuffer::setSize(const Nimble::Size &size)
  {
    m_d->m_size= size;

    // Resize all attachments
    for(GLenum attachment : m_d->m_renderBufferAttachments.keys()) {
      auto rb = renderBuffer(attachment);
      rb->setStorageFormat(size, rb->format(), rb->samples());
    }

    for(GLenum attachment : m_d->m_textureAttachments.keys()) {
      auto t = texture(attachment);
      t->setData(size.width(), size.height(), t->dataFormat(), 0);
    }
  }

  unsigned FrameBuffer::samples() const
  {
    return m_d->m_samples;
  }

  void FrameBuffer::setSamples(unsigned int samples)
  {
    m_d->m_samples = samples;

    // Change sample count for all render buffer attachments
    for(GLenum attachment : m_d->m_renderBufferAttachments.keys()) {
      auto rb = renderBuffer(attachment);
      rb->setStorageFormat(rb->size(), rb->format(), samples);
    }

    // Change sample count for all texture attachments
    for(GLenum attachment : m_d->m_textureAttachments.keys()) {
      auto t = texture(attachment);
      t->setSamples(samples);
    }
  }

  void FrameBuffer::attach(GLenum attachment, Luminous::RenderBuffer &buffer)
  {
    m_d->attach(attachment, buffer);
  }

  void FrameBuffer::attach(GLenum attachment, Luminous::Texture &texture)
  {
    m_d->attach(attachment, texture);
  }

  Luminous::Texture * FrameBuffer::texture(GLenum attachment) const
  {
    if(m_d->m_textureAttachments.contains(attachment))
      return RenderManager::getResource<Luminous::Texture>(m_d->m_textureAttachments.value(attachment));

    return nullptr;
  }

  Luminous::RenderBuffer * FrameBuffer::renderBuffer(GLenum attachment) const
  {
    if(m_d->m_renderBufferAttachments.contains(attachment))
      return RenderManager::getResource<Luminous::RenderBuffer>(m_d->m_renderBufferAttachments.value(attachment));

    return nullptr;
  }

  QList<GLenum> FrameBuffer::textureAttachments() const
  {
    return m_d->m_textureAttachments.keys();
  }

  QList<GLenum> FrameBuffer::renderBufferAttachments() const
  {
    return m_d->m_renderBufferAttachments.keys();
  }

  FrameBuffer::FrameBufferType FrameBuffer::targetType() const
  {
    return m_d->m_targetType;
  }

  FrameBuffer::FrameBufferBind FrameBuffer::targetBind() const
  {
    return m_d->m_targetBind;
  }

  void FrameBuffer::setTargetBind(FrameBufferBind target)
  {
    m_d->m_targetBind = target;
  }

  Texture & FrameBuffer::createTextureAttachment(GLenum attachment, const Luminous::PixelFormat & format)
  {
    return m_d->createTextureAttachment(attachment, format);
  }

  RenderBuffer & FrameBuffer::createRenderBufferAttachment(GLenum attachment, GLenum storageFormat)
  {
    return m_d->createRenderBufferAttachment(attachment, storageFormat);
  }
}
