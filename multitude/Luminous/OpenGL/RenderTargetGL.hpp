#ifndef LUMINOUS_RENDERTAGET_HPP
#define LUMINOUS_RENDERTAGET_HPP

#include "ResourceHandleGL.hpp"
#include "TextureGL.hpp"
#include "Luminous/RenderTarget.hpp"

#include <QSize>

namespace Luminous
{

  class RenderBufferGL : public ResourceHandleGL
  {
  public:
    RenderBufferGL(StateGL & state);
    RenderBufferGL(RenderBufferGL && buffer);
    ~RenderBufferGL();

    void sync(const RenderBuffer & buffer);

    void storageFormat(const QSize & size, GLenum format, int samples);

    void bind();
    void unbind();
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTargetGL : public ResourceHandleGL
  {
  public:
    RenderTargetGL(StateGL & state);
    RenderTargetGL(RenderTargetGL && target);

    ~RenderTargetGL();

    void sync(const RenderTarget & target);

    void attach(GLenum attachment, RenderBufferGL & renderBuffer);
    void attach(GLenum attachment, TextureGL & texture);

    void detach(GLenum attachment);

    void bind();
    void unbind();

    bool check();

  private:
    RenderTarget::RenderTargetType m_type;
    QSize m_size;
  };

}

#endif
