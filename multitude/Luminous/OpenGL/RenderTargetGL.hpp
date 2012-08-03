#ifndef LUMINOUS_RENDERTAGET_HPP
#define LUMINOUS_RENDERTAGET_HPP

#include "ResourceHandleGL.hpp"
#include "TextureGL.hpp"

#include <QSize>

namespace Luminous
{

  class RenderBufferGL : public ResourceHandleGL
  {
  public:
    RenderBufferGL(StateGL & state);
    ~RenderBufferGL();

    void storageFormat(const QSize & size, GLenum format, int samples);

    void bind();
    void unbind();

  private:

  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  class RenderTargetGL : public ResourceHandleGL
  {
  public:
    RenderTargetGL(StateGL & state);
    ~RenderTargetGL();

    void attach(GLenum attachment, RenderBufferGL & renderBuffer);
    void attach(GLenum attachment, TextureGL & texture);

    void detach(GLenum attachment);

    void bind();
    void unbind();

    bool check();
  };

}

#endif
