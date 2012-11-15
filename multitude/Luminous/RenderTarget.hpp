#ifndef LUMINOUS_RENDER_TARGET_HPP
#define LUMINOUS_RENDER_TARGET_HPP

#include "Export.hpp"
#include "RenderResource.hpp"
#include "Texture2.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Size.hpp>

namespace Luminous
{


  /// This class represents an off-screen render target that is optimized for use as a render target. This class should be used if you
  /// do not need to sample (e.g. use as a texture) your rendered image.
  class LUMINOUS_API RenderBuffer : public RenderResource
  {
  public:
    RenderBuffer();
    ~RenderBuffer();

    RenderBuffer(RenderBuffer & rb);
    RenderBuffer & operator=(RenderBuffer & rb);

    RenderBuffer(RenderBuffer && rb);
    RenderBuffer & operator=(RenderBuffer && rb);

    void storageFormat(const Nimble::Size &size, GLenum format, int samples);

    const Nimble::Size & size() const;
    GLenum format() const;
    int samples() const;

  private:
    class D;
    D * m_d;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  /// This class is an abstraction of a generic render target. It provides an
  /// abstraction of the OpenGL FrameBufferObject API.
  /// @todo document, add API to detach attachments, rename to framebuffer
  class LUMINOUS_API RenderTarget
      : public RenderResource, public Patterns::NotCopyable
  {
  private:
    class D;
    D * m_d;

    /// This class is a helper used to implement copying RenderTarget classes.
    /// You should never manually instantiate this class. It is also meant to be
    /// used with RenderTarget::deepCopy, RenderTarget::shallowCopy, and
    /// RenderTarget::shallowCopyNoAttachments functions.
    /// @sa RenderTarget
    class LUMINOUS_API RenderTargetCopy
    {
    private:
      RenderTargetCopy(D * d) : m_d(d) {}

      D * m_d;

      friend class RenderTarget;
    };

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

  public:
    enum RenderTargetType
    {
        INVALID
      , WINDOW
      , NORMAL
    };

    enum RenderTargetBind
    {
        BIND_DEFAULT
      , BIND_READ
      , BIND_DRAW
    };

    RenderTarget(RenderTargetType type = NORMAL);
    ~RenderTarget();

    RenderTarget(const RenderTargetCopy & rt);
    RenderTarget & operator=(const RenderTargetCopy & rt);

    RenderTarget(RenderTarget && rt);
    RenderTarget & operator=(RenderTarget && rt);

    RenderTargetCopy shallowCopyNoAttachments() const;
    RenderTargetCopy shallowCopy() const;
    RenderTargetCopy deepCopy() const;

    const Nimble::Size & size() const;
    void setSize(const Nimble::Size &size);

    unsigned samples() const;
    void setSamples(unsigned int samples);

    void attach(GLenum attachment, Luminous::Texture & texture);
    void attach(GLenum attachment, Luminous::RenderBuffer & buffer);

    Luminous::Texture & createTextureAttachment(GLenum attachment, const Luminous::PixelFormat & format);
    Luminous::RenderBuffer & createRenderBufferAttachment(GLenum attachment, GLenum storageFormat);

    Luminous::Texture * texture(GLenum attachment) const;
    Luminous::RenderBuffer * renderBuffer(GLenum attachment) const;

    QList<GLenum> textureAttachments() const;
    QList<GLenum> renderBufferAttachments() const;

    RenderTargetType targetType() const;

    RenderTargetBind targetBind() const;
    void setTargetBind(RenderTargetBind bind);
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderContext;

  class LUMINOUS_API RenderTargetGuard
  {
  public:
    RenderTargetGuard(RenderContext & r);
    ~RenderTargetGuard();

  private:
    RenderContext & m_renderContext;
  };
}

#endif
