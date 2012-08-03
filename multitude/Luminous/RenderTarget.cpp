#include "RenderTarget.hpp"

#include <QSize>

namespace Luminous
{
  class RenderBuffer::D
  {
  public:
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

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTarget::D
  {
  public:

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

  void RenderTarget::setSize(Nimble::Vector2i size)
  {

  }

  void RenderTarget::attach(GLenum attachment, Luminous::RenderBuffer &buffer)
  {

  }

  void RenderTarget::attach(GLenum attachment, Luminous::Texture &texture)
  {

  }

  Luminous::Texture * RenderTarget::texture(GLenum attachment)
  {

  }

  Luminous::RenderBuffer * RenderTarget::renderBuffer(GLenum attachment)
  {

  }
}
