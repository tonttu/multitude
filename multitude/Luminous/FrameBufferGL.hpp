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

  /// RenderBufferGL is the RenderBuffer representation in GPU memory.
  /// @sa RenderBuffer
  class RenderBufferGL : public ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state
    LUMINOUS_API RenderBufferGL(StateGL & state);
    /// Move constructor
    /// @param buffer render buffer to move
    LUMINOUS_API RenderBufferGL(RenderBufferGL && buffer);
    /// Destructor
    LUMINOUS_API ~RenderBufferGL();

    /// Synchronize the settings from the CPU object
    /// @param buffer buffer to synchronize from
    LUMINOUS_API void sync(const RenderBuffer & buffer);

    /// Set the storage format from the CPU object
    /// @param buffer buffer to get the settings from
    LUMINOUS_API void setStorageFormat(const RenderBuffer & buffer);

    /// Bind the render buffer
    LUMINOUS_API void bind();
    /// Unbind any render buffer
    LUMINOUS_API void unbind();

  private:
    int m_generation;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  /// FrameBufferGL is the FrameBuffer representation in GPU memory.
  /// @sa FrameBuffer
  class FrameBufferGL : public ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state
    LUMINOUS_API FrameBufferGL(StateGL & state);
    /// Move constructor
    /// @param target framebuffer to move
    LUMINOUS_API FrameBufferGL(FrameBufferGL && target);
    /// Destructor
    LUMINOUS_API ~FrameBufferGL();

    /// Synchronize the state of the GPU object to the given CPU object.
    /// @param target framebuffer object to sync to
    LUMINOUS_API void sync(const FrameBuffer & target);

    /// Attach a render buffer to the framebuffer
    /// @param attachment attachment to use
    /// @param renderBuffer render buffer to attach
    LUMINOUS_API void attach(GLenum attachment, RenderBufferGL & renderBuffer);
    /// Attach a texture to the framebuffer
    /// @param attachment attachment to use
    /// @param texture texture to attach
    LUMINOUS_API void attach(GLenum attachment, TextureGL & texture);
    /// Detach the specified attachment
    /// @param attachment attachment to detach
    LUMINOUS_API void detach(GLenum attachment);
    /// Bind the framebuffer
    LUMINOUS_API void bind();
    /// Unbind the framebuffer
    LUMINOUS_API void unbind();

    /// Check that the framebuffer object is valid. This function will output
    /// warnings to stderr if the framebuffer is not complete.
    /// @return true if the framebuffer is complete and can be rendered to; otherwise false
    LUMINOUS_API bool check();

  private:
    void syncImpl();
    void bindImpl();

  private:
    FrameBuffer::FrameBufferType m_type;
    FrameBuffer::FrameBufferBind m_bind;
    Nimble::Size m_size;
    int m_generation{-1};
    QMap<GLenum, RenderResource::Id> m_textureAttachments;
    QMap<GLenum, RenderResource::Id> m_renderBufferAttachments;
    bool m_dirty{true};
  };

}

#endif
