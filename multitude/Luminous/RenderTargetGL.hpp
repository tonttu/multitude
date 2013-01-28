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
    LUMINOUS_API RenderBufferGL(StateGL & state);
    LUMINOUS_API RenderBufferGL(RenderBufferGL && buffer);
    LUMINOUS_API ~RenderBufferGL();

    LUMINOUS_API void sync(const RenderBuffer & buffer);

    LUMINOUS_API void setStorageFormat(const RenderBuffer & buffer);

    LUMINOUS_API void bind();
    LUMINOUS_API void unbind();

  private:
    int m_generation;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTargetGL : public ResourceHandleGL
  {
  public:
    LUMINOUS_API RenderTargetGL(StateGL & state);
    LUMINOUS_API RenderTargetGL(RenderTargetGL && target);

    LUMINOUS_API ~RenderTargetGL();

    LUMINOUS_API void sync(const RenderTarget & target);

    LUMINOUS_API void attach(GLenum attachment, RenderBufferGL & renderBuffer);
    LUMINOUS_API void attach(GLenum attachment, TextureGL & texture);

    LUMINOUS_API void detach(GLenum attachment);

    LUMINOUS_API void bind();
    LUMINOUS_API void unbind();

    LUMINOUS_API bool check();

  private:
    RenderTarget::RenderTargetType m_type;
    RenderTarget::RenderTargetBind m_bind;
    Nimble::Size m_size;
  };

}

#endif
