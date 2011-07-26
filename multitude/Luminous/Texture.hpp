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

#ifndef LUMINOUS_TEXTURE_HPP
#define LUMINOUS_TEXTURE_HPP

#include <Luminous/Luminous.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/PixelFormat.hpp>

#include <Nimble/Vector2.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Valuable/HasValues.hpp>

namespace Luminous
{
  class PixelFormat;
  class Image;
  class CompressedImage;

  /// UploadLimiter manages GPU upload limits for each RenderThread per frame.
  /// These limits should be obeyed when loading data to GPU with glTexImage2D
  /// or similar command. All classes in Luminous follow this limit automatically.
  ///
  /// Simple example:
  /// @code
  /// long & limit = UploadLimiter::available();
  /// if(limit >= bytesToUpload) {
  ///   glTexImage2D(...);
  ///   limit -= bytesToUpload;
  /// } /* else wait for the next frame */
  /// @endcode
  class LUMINOUS_API UploadLimiter : public Valuable::HasValues
  {
  public:
    /// Accepts "frame" event, resets all limits for a new frame.
    void processMessage(const char * type, Radiant::BinaryData & data);

    /// Get the singleton instance. Usually not used, since all the important
    /// functions are already static.
    /// @return an instance of UploadLimiter
    static UploadLimiter & instance();

    /// Returns a reference to the thread-specific remaining upload capacity in
    /// bytes. Use this to check if some GPU transfer operation should be
    /// performed right away, delayed to the next frame or splitted to smaller
    /// pieces.
    /// After doing any upload operation, decrease this value accordingly.
    /// @returns How many bytes there are available for uploading in this frame.
    static long & available();

    /// Returns the current frame number.
    static long frame();

    /// Ask the frame upload limit.
    /// @return The frame upload limit in bytes.
    static long limit();

    /// Sets the upload limit
    /// @param limit New upload limit in bytes.
    static void setLimit(long limit);

    /// Enable or disable the upload limiter. available() will always return
    /// numeric_limits<long>::max() if limiter is disabled.
    /// @param v True if limiter should be enabled
    static void setEnabledForCurrentThread(bool v);

    /// Ask the limiter status
    /// @returns True if limiter is active for this thread.
    static bool enabledForCurrentThread();

  private:
    UploadLimiter();
    int m_frame;
    long m_frameLimit;
    bool m_inited;
  };

  /// Base class for different textures
  /** Texture objects can be create without a valid OpenGL context, but their actual
      usage requires a valid OpenGL context. */
  template<GLenum TextureType>
  class LUMINOUS_API TextureT : public GLResource, public Patterns::NotCopyable
  {
    friend class Framebuffer;

  public:
    /// Constructs a texture and adds it to the given resource collection
    TextureT(GLResources * res = 0)
    : GLResource(res),
      m_textureId(0),
      m_width(0),
      m_height(0),
      m_srcFormat(PixelFormat::LAYOUT_UNKNOWN, PixelFormat::TYPE_UNKNOWN),
      m_internalFormat(0),
      m_haveMipmaps(false),
      m_consumed(0)
    {}
    virtual ~TextureT();

    /// Allocates the texture object. Does not allocate memory for the texture data.
    void allocate();

    /** Activate textureUnit and bind this texture to that unit.
    @param textureUnit texture unit to bind to*/
    void bind(GLenum textureUnit)
    {
      allocate();
      glActiveTexture(textureUnit);
      glBindTexture(TextureType, m_textureId);
    }

    /** Bind this texture to the currently active texture unit. */
    void bind()
    {
      allocate();
      glBindTexture(TextureType, m_textureId);
    }

    /// Returns the width of the texture (if known)
    int width() const { return m_width; }
    /// Returns the height of the texture (if known)
    int height() const { return m_height; }

    /// Returns the size (width x height) of the texture, if known
    Nimble::Vector2i size() const
    { return Nimble::Vector2i(m_width, m_height); }

    /** Sets the width of the texture. This is not used by the object for
    anything but allows the user to query texture dimension from the texture
    object.
    @param w texture width */
    void setWidth(int w) { m_width = w; }
    /** Sets the height of the texture. This is not used by the object for
    anything but allows the user to query texture dimension from the texture
    object.
    @param h texture height */
    void setHeight(int h) { m_height = h; }

    /// Get the number of pixels in the texture.
    /// Computes the area of the texture in pixels
    /// @return the number of pixels in the texture
    int pixelCount() const { return m_width * m_height; }

    /// Returns estimation of much GPU RAM the texture uses.
    /// @return estimated number of bytes
    virtual long consumesBytes()
    {
      /// @todo how about compressed formats with mipmaps, does the 4/3 rule apply here as well?
      if(m_consumed > 0) return m_consumed;
      /// @todo this is wrong, should use m_internalFormat to calculate the size
      float used = float(m_width) * m_height * m_srcFormat.bytesPerPixel();
      // Mipmaps used 33% more memory
      used *= (m_haveMipmaps ? (4.f / 3.f) : 1.f);
      return (long)used;
    }

    /// Get the OpenGL texture id
    /// @return the OpenGL texture id
    GLuint id() const { return m_textureId; }
    /// Returns true if the texture is defined
    /// @return true if the texture object has been defined
    bool isDefined() const { return id() != 0; }

    /// Get the pixel format of the source data.
    /// Returns the pixel format of the source data, not the internal pixel format.
    /// @see internalFormat()
    /// @return pixel format of the source data
    const PixelFormat & srcFormat() const { return m_srcFormat; }
    /// Get the internal format of the texture
    /// @return internal OpenGL texture format
    GLenum internalFormat() const { return m_internalFormat; }

  protected:
    /// OpenGL texture handle
    GLuint m_textureId;
    /// Width of the texture
    int m_width;
    /// Height of the texture
    int m_height;
    /// Pixel format of the source data, not the internal pixel format
    PixelFormat m_srcFormat;
    /// The internal texture format
    GLenum m_internalFormat;
    /// Does the texture have mipmaps
    bool m_haveMipmaps;
    /// The actual consumed size on GPU, if good enough estimate is known
    /// If this is zero, the size is guessed from internalFormat and size
    int m_consumed;
  };

  /// A 1D texture
  class LUMINOUS_API Texture1D : public TextureT<GL_TEXTURE_1D>
  {
  public:
    /// Constructs a 1D texture and adds it to the given resource collection
    Texture1D(GLResources * resources = 0) : TextureT<GL_TEXTURE_1D> (resources) {}

    /// Load the texture from from raw data, provided by the user
    bool loadBytes(GLenum internalFormat, int h,
                   const void* data,
                   const PixelFormat& srcFormat,
                   bool buildMipmaps = true);

    /// Constructs a 1D texture by loading it from a file
    static Texture1D* fromImage(Image & image, bool buildMipmaps = true, GLResources * resources = 0);
    /// Constructs a 1D texture by loading it from memory
    static Texture1D* fromBytes(GLenum internalFormat,
                                int h,
                                const void* data,
                                const PixelFormat& srcFormat, bool buildMipmaps = true,
                                GLResources * resources = 0);

  };

  /// A 2D texture
  class LUMINOUS_API Texture2D : public TextureT<GL_TEXTURE_2D>
  {
  public:
    /// Constructs a 2D texture and adds it to the given resource collection
    Texture2D(GLResources * resources = 0) :
        TextureT<GL_TEXTURE_2D>(resources), m_loadedLines(0) {}

    /// Get the aspect ratio of the texture
    /// Returns the aspect ratio of this texture, ie. the ratio of width to height
    /// @return the aspect ratio
    float aspectRatio() const { return m_width / (float) m_height; }

    /// Load the texture from an image file
    bool loadImage(const char * filename, bool buildMipmaps = true);
    /// Load the texture from an image
    /// @param image image to generate the texture from
    /// @param buildMipmaps if true, generate mipmaps automatically
    /// @param internalFormat specify the internal OpenGL texture format. If
    /// zero, set the format automatically
    /// @return true on success
    bool loadImage(const Luminous::Image & image, bool buildMipmaps = true, int internalFormat = 0);
    /// Load the texture from a compressed image
    /// @param image compressed image to load from
    /// @return true on success
    bool loadImage(const Luminous::CompressedImage & image);

    /// Load the texture from raw data, provided by the user
    bool loadBytes(GLenum internalFormat, int w, int h,
                   const void* data,
                   const PixelFormat& srcFormat,
                   bool buildMipmaps = true);
    /// Load a sub-texture.
    void loadSubBytes(int x, int y, int w, int h, const void * subData);

    /// Load some lines to the texture:
    void loadLines(int y, int h, const void * data, const PixelFormat& srcFormat);

    /// Create a new texture, from an image file
    static Texture2D * fromFile(const char * filename, bool buildMipmaps = true, GLResources * resources = 0);
    /// Create a new texture, from an image
    static Texture2D * fromImage(Luminous::Image & img, bool buildMipmaps = true, GLResources * resources = 0);
    /// Create a new texture from raw image data, provided by the user
    static Texture2D * fromBytes(GLenum internalFormat, int w, int h,
                const void * data,
                const PixelFormat& srcFormat,
                bool buildMipmaps = true, GLResources * resources = 0);
    /// Get the number of scan-lines that have been uploaded to the GPU
    /// This is used for example while doing progressive loading of images.
    /// @return number of uploaded lines
    inline unsigned loadedLines() const { return m_loadedLines; }
  private:
    unsigned m_loadedLines;
  };

  /// A 3D texture
  class LUMINOUS_API Texture3D : public TextureT<GL_TEXTURE_3D>
  {
  public:
    /// Constructs a 3D texture and adds it to the given resource collection
    /// @param resources resource collection to own this texture
    Texture3D(GLResources * resources = 0)
      : TextureT<GL_TEXTURE_3D> (resources),
      m_depth(0)
    {}

    /// Set the depth of the 3D texture (ie. the z dimension)
    /// @param d new depth
    void setDepth(int d) { m_depth = d; }
    /// Get the depth of the 3D texture
    /// @return depth of the texture
    int depth() const { return m_depth; }
  private:
    int m_depth;
  };

  /// A cubemap texture
  class LUMINOUS_API TextureCube : public TextureT<GL_TEXTURE_CUBE_MAP>
  {
  public:
    /// Constructs a cube texture and adds it to the given resource collection
    TextureCube(GLResources * resources = 0)
        : TextureT<GL_TEXTURE_CUBE_MAP> (resources) {}

  };

}

#endif
