/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Luminous/VertexBuffer.hpp>

#include <Nimble/Math.hpp>

#include <Radiant/Trace.hpp>

#include "RenderContext.hpp"

namespace Luminous
{
  using namespace Radiant;

  template<GLenum type>
  BufferObject<type>::BufferObject(Luminous::RenderContext * resources)
    : GLResource(resources),
    m_filled(0)
  {
    // info("BufferObject<type>::BufferObject # %p", this);
    glGenBuffers(1, &m_bufferId);
    setPersistent(true);
  }

  template<GLenum type>
  BufferObject<type>::~BufferObject()
  {
    // info("BufferObject<type>::~BufferObject # %p", this);
    glDeleteBuffers(1, &m_bufferId);
  }

  template<GLenum type>
  void BufferObject<type>::allocate(size_t bytes, Usage usage)
  {
    fill(0, bytes, usage);
    m_filled = bytes;
  }

  template<GLenum type>
  void BufferObject<type>::bind()
  {
    context()->bindBuffer(type, m_bufferId);
  }

  template<GLenum type>
  void BufferObject<type>::unbind()
  {
    context()->bindBuffer(type, 0);
  }

  template<GLenum type>
  void BufferObject<type>::fill(const void * data, size_t bytes, Usage usage)
  {
    m_filled = bytes;

    bind();

    if(bytes)
      glBufferData(type, bytes, data, usage);
  }

  template<GLenum type>
  void BufferObject<type>::partialFill(size_t offsetInBytes, const void * data, size_t bytes)
  {
    bind();

    glBufferSubData(type, offsetInBytes, bytes, data);
    m_filled = Nimble::Math::Max(m_filled, offsetInBytes + bytes);
  }

#ifndef LUMINOUS_OPENGLES
  template<GLenum type>
  void BufferObject<type>::read(Nimble::Vector2i size, Nimble::Vector2i pos,
                                Luminous::PixelFormat pix, Usage usage)
  {
    bind();

    size_t bytes = pix.bytesPerPixel()*size.x*size.y;
    if(m_filled < bytes) {
      m_filled = bytes;
      glBufferData(type, bytes, 0, usage);
    }

    glReadPixels(pos.x, pos.y, size.x, size.y, pix.layout(), pix.type(), 0);
  }

  template<GLenum type>
  void * BufferObject<type>::map(AccessMode mode)
  {
    bind();

    return glMapBuffer(type, mode);
  }

  template<GLenum type>
  void BufferObject<type>::unmap()
  {
    bind();

    glUnmapBuffer(type);
  }
#endif // LUMINOUS_OPENGLES

}
