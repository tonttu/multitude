#ifndef LUMINOUS_RENDER_TARGET_HPP
#define LUMINOUS_RENDER_TARGET_HPP

#include "Export.hpp"
#include "RenderResource.hpp"
#include "Texture2.hpp"

#include <Nimble/Vector2.hpp>

#include <QSize>

namespace Luminous
{

  class RenderBuffer : public RenderResource
  {
  public:
    RenderBuffer();
    ~RenderBuffer();

    void storageFormat(const QSize & size, GLenum format);

  private:
    class D;
    D * m_d;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTarget : public RenderResource
  {
  public:
    RenderTarget();
    ~RenderTarget();

    void setSize(Nimble::Vector2i size);

    void attach(GLenum attachment, Luminous::Texture & texture);
    void attach(GLenum attachment, Luminous::RenderBuffer & buffer);

    Luminous::Texture * texture(GLenum attachment);
    Luminous::RenderBuffer * renderBuffer(GLenum attachment);

  private:
    class D;
    D * m_d;
  };

}

#endif
