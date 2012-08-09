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

#ifndef LUMINOUS_IMAGE_HPP
#define LUMINOUS_IMAGE_HPP

#include <Luminous/ContextVariable.hpp>
#include <Luminous/Export.hpp>
#include <Luminous/PixelFormat.hpp>
#include <Luminous/Texture.hpp>

#include <cstdio>
#include <vector>

namespace Luminous
{
  class CodecRegistry;

  /// Simple struct containing basic image information that can be quickly
  /// queried (with Image::ping) without loading the full image.
  struct ImageInfo {
    ImageInfo() : width(0), height(0), mipmaps(1) {}
    /// Width of the image
    int width;
    /// Height of the image
    int height;
    /// Number of embedded mipmaps, including the base image (1 if no mipmaps included)
    int mipmaps;
    /// Pixel format of the image
    PixelFormat pf;
  };

  /// Simple image handling class
  /** This class is used mainly to load/save image data in various formats.
      It provides minimal image processing support (resizing).

      The image data is stored in a single, straightforward buffer.
  */
  class LUMINOUS_API Image
  {
  public:
    Image();
    /// Constructs a deep copy
    Image(const Image& img);
    virtual ~Image();

    /// Allocates memory, for an image of given size and format
    void allocate(int width, int height, const PixelFormat & pf);

    /// Returns the aspect ration of the image (if defined)
    float aspect() const { return (float)m_width / (float)m_height; }

    /// The width ofthe image in pixels
    int width() const { return m_width; }
    /// The height of the image in pixels
    int height() const { return m_height; }
    /// The size of the image in pixels
    Nimble::Vector2i size() const
    { return Nimble::Vector2i(m_width, m_height); }
    /// The number of bytes a single line in the image takes
    int lineSize() const { return m_width * m_pixelFormat.bytesPerPixel(); }
    /// Returns a pointer to a specific line
    unsigned char* line(unsigned y) { return &m_data[y * lineSize()]; }

    const unsigned char* line(unsigned y) const { return &m_data[y * lineSize()]; }

    /// Returns a pointer to the image data
    unsigned char * bytes() { return & m_data[0]; }
    /// Returns a const pointer to the image data
    const unsigned char * bytes() const { return & m_data[0]; }

    /// Returns a pointer to the image data
    unsigned char * data() { return & m_data[0]; }
    /// Returns a const pointer to the image data
    const unsigned char * data() const { return & m_data[0]; }

    /// Check if a file is readable, and returns its core information
    static bool ping(const QString & filename, ImageInfo & info);

    /** Read a file to this Image object.
    @param filename name of the file to read from
    @return true if the image was successfully read */
    bool read(const char * filename);
    /** Write this Image to a file.
    @param filename name of the file to write to
    @return true if the image was successfully written*/
    bool write(const char * filename) const;

    /** Create an image object from data provided by the user.
    @param bytes pointer to image data
    @param width width of the image data
    @param height height of the image data
    @param format pixel format of the image data*/
    void fromData(const unsigned char * bytes, int width, int height,
          PixelFormat format);

    /// Returns the pixel format of the image
    const PixelFormat& pixelFormat() const { return m_pixelFormat; }

    /// Sets the new format and converts the image data to new format if necessary
    /// @param format new pixel format for the image
    /// @returns Returns true if the conversion was successful
    bool setPixelFormat(const PixelFormat & format);

    /// Clears the image, freeing the data.
    void clear();

    /// Returns true if the image does not contain any data.
    bool empty() const { return (m_data == 0); }

    /// Flip the image upside down
    void flipVertical();

    /** Resample a source image using straightforward bilinear
    interpolation.
    @param source image to resample
    @param w new width
    @param h new height
    @return true if the resamping succeed
    */
    bool copyResample(const Image & source, int w, int h);

    void minify(const Image & src, int w, int h);

    /** Down-sample the given image to quarter size.
    @param source image to resample
    @return true if resampling succeeded */
    bool quarterSize(const Image & source);
    /** Remove pixels from the right edge of the image. Works for RGB images.
    @param n number of pixels to remove
    @return false if the image is of unsupported pixel format*/
    bool forgetLastPixels(int n);
    /** Remove lines from the bottom of the image.
    @param n number of lines to remove */
    void forgetLastLines(int n);
    /** Removes the last line from the image. */
    void forgetLastLine();
    /** Checks that the image dimensions are efasible for a texture. In practice
        this functions removes pixels from the right and bottom, to make the
        width and height mutiples of two.
    */
    void makeValidTexture();
    /** Returns true if the image has an alpha channel.
        @return true if the image has alpha channel */
    bool hasAlpha() const;

    /** Makes a deep copy of the given image.
    @param img image to copy
    @return reference to this image */
    Image & operator = (const Image& img);

    /// Returns a pointer to the file-format codecs.
    /// @return pointer to the codec registry
    static CodecRegistry * codecs();

    /// Returns the alpha value [0,255] for the given relative coordinates in the image.
    /// @param relativeCoord relative pixel coordinate x,y in [0,1]
    /// @return alpha of the given pixel or 255 if image does not have alpha channel
    unsigned char pixelAlpha(Nimble::Vector2 relativeCoord) const;

    /// Fills the image with zeros
    void zero();

    /// Gets the color of a given pixel.
    /** The color is normalized, with each component in range 0-1.
    @param x pixel x coordinate
    @param y pixel y coordinate
    @return color at the given pixel */
    Nimble::Vector4 pixel(unsigned x, unsigned y) const;

    Nimble::Vector4 safePixel(int x, int y) const;

    void setPixel(unsigned x, unsigned y, const Nimble::Vector4 & pixel);

    /// Increments the generation count.
    /// This function should be called when the image has been modified.
    void changed() { m_generation++; }
    /** The generation count of the image object The generation count can be
    used to indicate that the image has changed, and one should update the
    corresponding OpenGL texture wo match the same generation. @return current
    generation count*/
    size_t generation() const { return m_generation; }

    /// Get a texture object based on the image
    /// @return texture matching the image
    Luminous::Texture & texture();

  protected:

    /// Width of the image in pixels
    int m_width;
    /// Height of the image in pixels
    int m_height;
    /// Pixel format of the image data
    PixelFormat m_pixelFormat;
    /// Pointer to the raw image data
    /// @todo change to QByteArray to get copy-on-write
    unsigned char* m_data;
    /// Generation count of the image used to indicate changes in the image
    /// data to determine when associated textures should be updated.
    size_t m_generation;

  private:
    std::unique_ptr<Texture> m_texture;
  };

  /** ImageTex is a utility class for rendering images.

  ImageTex provides an easy way to create OpenGL textures from image files in a
  way that handles multiple rendering contexts transparently.
  @deprecated This class will be removed in Cornerstone 2.1. Use Luminous::Image::texture() instead.
  */
  class LUMINOUS_API ImageTex : public Luminous::Image, public Luminous::ContextVariableT<Luminous::Texture2D>
  {
  public:
    MULTI_ATTR_DEPRECATED("This class is obsolete. Use Image::texture() instead.", ImageTex());

    /** Binds a texture representing this image to the current OpenGL context.

        @param textureUnit The OpenGL texture unit to bind to
        @param withmipmaps Should we use mimaps, or not. This argument only
        makes difference the first time this function executed for the context
        (and the texture is created), after that the the same texture is used.
        @return true if the bind succeeded
    */
    bool bind(GLenum textureUnit = GL_TEXTURE0, bool withmipmaps = true) {
      return bind(0, textureUnit, withmipmaps);
    }

    /** Binds a texture representing this image to the current OpenGL context.

        @param resources The OpenGL resource handler
        @param textureUnit The OpenGL texture unit to bind to
        @param withmipmaps Should we use mimaps, or not. This argument only
        makes difference the first time this function executed for the context
        (and the texture is created), after that the the same texture is used.
        @param internalFormat internal OpenGL texture format. May be zero to
        let the GPU automatically choose one.
        @return true if the bind succeeded
    */
    bool bind(RenderContext * resources, GLenum textureUnit = GL_TEXTURE0,
              bool withmipmaps = true, int internalFormat = 0);

    /// Checks if the image data is fully loaded to the GPU, inside a texture
    /// @param resources OpenGL resource collection to use
    /// @return true if the texture data has been fully uploaded to the GPU
    bool isFullyLoadedToGPU(RenderContext * resources = 0);

    ImageTex & operator = (const Luminous::Image & that)
    {
      Image::operator =(that);

      return *this;
    }

    /// Creates a new ImageTex from this. All the cpu data from Luminous::Image
    /// is moved to the new object.
    /// @return cloned ImageTex
    ImageTex * move();
  };

#ifndef LUMINOUS_OPENGLES

  /// A compressed image. Currently supports DXT format.
  class LUMINOUS_API CompressedImage
  {
  public:
    CompressedImage();
    virtual ~CompressedImage();

    /// Clears the image data and release any allocated memory
    void clear();

    /// Reads an image from a file
    /// @param filename filename to load
    /// @param level mipmap level to load
    /// @return true if succeed, false on error
    bool read(const QString & filename, int level = 0);
    /// Loads image data from the given file handle
    /// @param file file to read from
    /// @param info image info
    /// @param size bytes to read
    /// @return true if the reading succeeded, false otherwise
    bool loadImage(FILE * file, const ImageInfo & info, int size);
    /// Returns a pointer to the raw image data
    void * data() const;
    /// Returns the size of the image data in bytes
    /// @return image data size in bytes
    int datasize() const;

    /// Returns the widget of the image
    int width() const { return m_size.x; }
    /// Returns the height of the image
    int height() const { return m_size.y; }

    /// Returns the compression used
    PixelFormat::Compression compression() const { return m_compression; }

    /// Returns the alpha in the given pixel position
    float readAlpha(Nimble::Vector2i pos) const;

  protected:
    /// Size of the image in pixels
    Nimble::Vector2i m_size;
    /// Used compression
    PixelFormat::Compression m_compression;

/// @cond
    class Private;
    Private* m_d;
/// @endcond
  };

  /// CompressedImageTex provides an easy way to access textures generated from
  /// compressed images. @sa Luminous::ImageTex
  class LUMINOUS_API CompressedImageTex : public CompressedImage, public Luminous::ContextVariableT<Luminous::Texture2D>
  {
  public:
    virtual ~CompressedImageTex();

    /** Binds a texture representing this compressed image to the current
    OpenGL context.
    @param resources The OpenGL resource handler
    @param textureUnit The OpenGL texture unit to bind to*/
    void bind(RenderContext * resources, GLenum textureUnit = GL_TEXTURE0);

    /// Creates a new CompressedImageTex from this object. All the cpu data from
    /// Luminous::CompressedImage is moved to the new object.
    /// @return new CompressedImageTex object
    CompressedImageTex * move();
  };
#endif // LUMINOUS_OPENGLES
}

#endif
