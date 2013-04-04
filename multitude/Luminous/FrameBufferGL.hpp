/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RENDERTAGET_HPP
#define LUMINOUS_RENDERTAGET_HPP

#include "ResourceHandleGL.hpp"
#include "TextureGL.hpp"
#include "Luminous/FrameBuffer.hpp"

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

  class FrameBufferGL : public ResourceHandleGL
  {
  public:
    LUMINOUS_API FrameBufferGL(StateGL & state);
    LUMINOUS_API FrameBufferGL(FrameBufferGL && target);

    LUMINOUS_API ~FrameBufferGL();

    LUMINOUS_API void sync(const FrameBuffer & target);

    LUMINOUS_API void attach(GLenum attachment, RenderBufferGL & renderBuffer);
    LUMINOUS_API void attach(GLenum attachment, TextureGL & texture);

    LUMINOUS_API void detach(GLenum attachment);

    LUMINOUS_API void bind();
    LUMINOUS_API void unbind();

    LUMINOUS_API bool check();

  private:
    FrameBuffer::FrameBufferType m_type;
    FrameBuffer::FrameBufferBind m_bind;
    Nimble::Size m_size;
  };

}

#endif
