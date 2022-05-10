/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_RENDER_TARGET_HPP
#define LUMINOUS_RENDER_TARGET_HPP

#include "Export.hpp"
#include "RenderResource.hpp"
#include "Texture.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Size.hpp>

namespace Luminous
{


  /// This class represents an off-screen render target that is optimized for
  /// use as a render target. This class should be used if you do not need to
  /// sample (e.g. use as a texture) your rendered image.
  /// GPU correspondent of this class is RenderBufferGL
  class LUMINOUS_API RenderBuffer : public RenderResource
  {
  public:
    /// Construct a new RenderBuffer
    RenderBuffer();
    /// Destructor
    ~RenderBuffer();

    /// Construct a copy of the given RenderBuffer
    /// @param rb buffer to copy
    RenderBuffer(RenderBuffer & rb);
    /// Copy the given RenderBuffer
    /// @param rb buffer to copy
    RenderBuffer & operator=(RenderBuffer & rb);

    /// Move constructor
    /// @param rb buffer to move
    RenderBuffer(RenderBuffer && rb);
    /// Move the given RenderBuffer
    /// @param rb buffer to move
    RenderBuffer & operator=(RenderBuffer && rb);

    /// Set the data storage, format, dimensions and sample count of the
    /// RenderBuffer's buffer
    /// @param size dimensions of the buffer
    /// @param format data format
    /// @param samples buffer sample count
    void setStorageFormat(const Nimble::Size &size, GLenum format, int samples);

    /// Get the dimensions of the buffer
    /// @return dimensions of the buffer
    const Nimble::Size & size() const;
    /// Get the buffer format
    /// @return format of the buffer
    GLenum format() const;
    /// Get the sample count of the buffer
    /// @return buffer sample count
    int samples() const;

  private:
    class D;
    D * m_d;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  /// This class is an abstraction of a generic render target. It provides an
  /// abstraction of the OpenGL FrameBufferObject API.
  /// GPU correspondent of this class is FrameBufferGL.
  /// @todo add API to detach attachments, rename to framebuffer
  class LUMINOUS_API FrameBuffer
      : public RenderResource, public Patterns::NotCopyable
  {
  public:
    /// Type of FrameBuffer
    enum FrameBufferType
    {
      /// Invalid is used in case of error
      INVALID,
      /// This object represents back buffer of window
      WINDOW,
      /// This render target has textures or manually allocated render buffers as its target buffers
      NORMAL
    };

    /// How FrameBuffer is bound. Determines what to do when target is bound
    enum FrameBufferBind
    {
      /// Bind into both read and draw targets
      BIND_DEFAULT,
      /// Bind only for read target
      BIND_READ,
      /// Bind only for draw target
      BIND_DRAW
    };

    /// Constructor of FrameBuffer
    /// @param type Type of the render target
    FrameBuffer(FrameBufferType type = NORMAL);
    /// Destructor
    ~FrameBuffer();

    /// Move constructor
    /// @param rt Render target to move
    FrameBuffer(FrameBuffer && rt);
    /// Move assignment operator
    /// @param rt FrameBuffer to move
    /// @return Reference to this
    FrameBuffer & operator=(FrameBuffer && rt);

    /// Size of the render target. Each attachment has this as theirs size
    /// @sa setSize
    /// @return Size of the target
    const Nimble::Size & size() const;
    /// Sets size for this render target
    /// @sa size
    /// @param size Size for attachments
    void setSize(const Nimble::Size &size);

    /// Number of samples if using multisampling. Zero if multisampling is disabled.
    /// @sa setSamples
    /// @return Number of samples
    unsigned samples() const;
    /// Sets number of samples for multisampling. Zero disables multisampling
    /// @param samples Number of samples to take
    /// @sa samples
    void setSamples(unsigned int samples);

    /// Attach a Texture to this render target
    /// @param attachment Attachment point (f. ex. GL_COLOR_ATTACHMENT0)
    /// @param texture Texture for attachment
    void attach(GLenum attachment, Luminous::Texture & texture);
    /// Attach a RenderBuffer to this render target
    /// @param attachment Attachment point (f. ex. GL_COLOR_ATTACHMENT0)
    /// @param buffer RenderBuffer to attach
    void attach(GLenum attachment, Luminous::RenderBuffer & buffer);

    /// Creates Texture and attaches it to given slot
    /// @param attachment Slot to attach newly created texture
    /// @param format Format of the texture to be created
    /// @return Reference to the created texture
    Luminous::Texture & createTextureAttachment(GLenum attachment, const Luminous::PixelFormat & format);
    /// Creates RenderBuffer and attaches it to given slot
    /// @param attachment Slot to attach newly created render buffer
    /// @param storageFormat Format of the render buffer to be created
    /// @return Reference to the created render buffer
    Luminous::RenderBuffer & createRenderBufferAttachment(GLenum attachment, GLenum storageFormat);

    /// Returns Texture attached for given slot. If no texture is attached here returns nullptr.
    /// @param attachment Attachment slot to be searched.
    /// @return Pointer to the attachment. If none found nullptr is returned.
    Luminous::Texture * texture(GLenum attachment) const;
    /// Returns RenderBuffer attached for given slot. If no render buffer is attached here returns nullptr.
    /// @param attachment Attachment slot to be searched.
    /// @return Pointer to the attachment. If none found nullptr is returned.
    Luminous::RenderBuffer * renderBuffer(GLenum attachment) const;

    /// Returns list of attachments where Texture is used.
    /// @return Enumerations textures attached
    QMap<GLenum, RenderResource::Id> textureAttachments() const;
    /// Returns list of attachments where RenderBuffer is used.
    /// @return Enumerations for render buffer attachments
    QMap<GLenum, RenderResource::Id> renderBufferAttachments() const;

    /// Returns type of this render target.
    /// @return Type of this target.
    FrameBufferType targetType() const;

    /// Returns the current binding type for this target.
    /// @sa setTargetBind
    /// @return How to bind this target.
    FrameBufferBind targetBind() const;
    /// Sets the binding type for this target.
    /// @param bind How to bind this target.
    /// @sa bind
    void setTargetBind(FrameBufferBind bind);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}

#endif
