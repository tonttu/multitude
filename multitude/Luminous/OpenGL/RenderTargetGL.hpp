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

    void storageFormat(const RenderBuffer & buffer);

    void bind();
    void unbind();

  private:
    int m_generation;
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
    RenderTarget::RenderTargetBind m_bind;
    Nimble::Size m_size;
  };

}

#endif
