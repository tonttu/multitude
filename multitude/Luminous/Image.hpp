/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_IMAGE_HPP
#define LUMINOUS_IMAGE_HPP

#include <Luminous/Export.hpp>
#include <Luminous/PixelFormat.hpp>

#include <Radiant/Mutex.hpp>

#include <Nimble/Size.hpp>
#include <Nimble/Vector4.hpp>

#include <QFile>

#include <cstdio>
#include <memory>
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
    /// Construct an empty image
    Image();
    /// Copy constructor
    /// @param img image to copy
    Image(const Image& img);
    /// Move constructor
    /// @param img image to move
    Image(Image && img);
    /// Destructor
    virtual ~Image();

    /// Allocates memory, for an image of given size and format
    /// @param width width in pixels
    /// @param height height in pixels
    /// @param pf pixel format
    /// @returns false if memory allocation fails
    bool allocate(int width, int height, const PixelFormat & pf);

    /// Get the aspect ratio (width / height) of the image
    /// @return aspect ratio
    float aspect() const { return (float)m_width / (float)m_height; }

    /// Get the image width
    /// @return width in pixels
    int width() const { return m_width; }
    /// Get the image height
    /// @return height in pixels
    int height() const { return m_height; }
    /// Get the image size
    /// @return image size in pixels
    Nimble::Size size() const
    { return Nimble::Size(m_width, m_height); }
    /// The number of bytes a single line in the image takes
    /// @return line size in bytes
    int lineSize() const { return m_lineSize ? m_lineSize : m_width * m_pixelFormat.bytesPerPixel(); }
    /// Get a pointer to image data on specific line
    /// @param y line to query for
    /// @return pointer to beginning of the line
    unsigned char* line(unsigned y) { return &m_data[y * lineSize()]; }
    /// @copydoc line
    const unsigned char* line(unsigned y) const { return &m_data[y * lineSize()]; }

    /// Get a pointer to image data
    /// @return pointer to image data
    unsigned char * bytes() { return m_data; }
    /// @copydoc bytes
    const unsigned char * bytes() const { return m_data; }

    /// @copydoc bytes
    unsigned char * data() { return bytes(); }
    /// @copydoc bytes
    const unsigned char * data() const { return bytes(); }

    /// Set the data pointer without copying the actual data. The caller must
    /// not delete the data while this Image object is still using it.
    /// @param lineSize line size in bytes.
    void setData(unsigned char * bytes, int width, int height,
                 PixelFormat format, int lineSize);

    /// Get basic image information from a file. This function does not decode
    /// the actual image data, typically just the header.
    /// @param filename filename to query
    /// @param[out] info image information
    /// @return true if the operation was successful; otherwise false
    static bool ping(const QString & filename, ImageInfo & info);

    /// Load an image from the given filename.
    /// @param filename name of the file to read from
    /// @return true if the image was successfully read
    bool read(const QString & filename, bool usePreMultipliedAlpha);
    /// Save the image to a file
    /// @param filename name of the file to write to
    /// @return true if the image was successfully written
    bool write(const QString & filename) const;

    /** Create an image object from data provided by the user. This function
     *  will copy the data, use setData if you don't want that to happen.
    @param bytes pointer to image data
    @param width width of the image data
    @param height height of the image data
    @param format pixel format of the image data
    @returns false if memory allocation fails */
    bool fromData(const unsigned char * bytes, int width, int height,
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
    /// @return true if the data is nullptr; otherwise false
    bool isEmpty() const { return (m_data == nullptr); }

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

    /// Scale an image to a smaller size. The given width and height should be
    /// smaller than in the source image.
    /// @param src source image
    /// @param w new width
    /// @param h new height
    void minify(const Image & src, int w, int h);

    /** Down-sample the given image to quarter size.
    @param source image to resample
    @return true if resampling succeeded */
    bool quarterSize(const Image & source);

    /** Returns true if the image has an alpha channel.
        @return true if the image has alpha channel */
    bool hasAlpha() const;

    /** Makes a deep copy of the given image.
    @param img image to copy
    @return reference to this image */
    Image & operator = (const Image& img);

    /// Move operator
    /// @param img image to move
    /// @return reference to this image
    Image & operator = (Image && img);

    /// Returns a pointer to the file-format codecs.
    /// @return pointer to the codec registry
    static CodecRegistry * codecs();

    /// Returns the alpha value [0,255] for the given relative coordinates in the image.
    /// @param relativeCoord relative pixel coordinate x,y in [0,1]
    /// @return alpha of the given pixel or 255 if image does not have alpha channel
    unsigned char pixelAlpha(Nimble::Vector2 relativeCoord) const;

    /// Fills the image with zeros
    void zero();

    /// Gets the color of a given pixel. The color is normalized, with each
    /// component in range 0-1.
    /// @param x pixel x coordinate
    /// @param y pixel y coordinate
    /// @return color at the given pixel
    Nimble::Vector4f pixel(int x, int y) const;

    /// Get a pixel from the image. This function does additional checks for
    /// make sure the requested pixels are within the image. If the requested
    /// pixel falls outside the image, transparent color is returned. The color
    /// is normalized.
    /// @param x pixel x coordinate
    /// @param y pixel y coordinate
    /// @return pixel color
    Nimble::Vector4f safePixel(int x, int y) const;

    /// Set a pixel to given color. The color must be normalized.
    /// @param x x coordinate of the pixel
    /// @param y y coordinate of the pixel
    /// @param pixel pixel color
    void setPixel(unsigned x, unsigned y, const Nimble::Vector4f & pixel);

    /// Increments the generation count.
    /// This function should be called when the image has been modified.
    void changed() { m_generation++; }

    /// The generation count of the image object The generation count can be used
    /// to indicate that the image has changed, and one should update the
    /// corresponding OpenGL texture wo match the same generation.
    /// @return current generation count
    size_t generation() const { return m_generation; }

    /// Get a texture object based on the image
    /// This function is thread-safe so it can be used in render functions
    /// @return texture matching the image
    Luminous::Texture & texture() const;

    /// @copydoc texture()
    const Luminous::Texture & constTexture() const;

    /// Check if the image associated texture has been initialized. The texture
    /// is initialized lazily when calling texture() method.
    /// @return true if the texture has been created
    /// @sa texture
    bool hasTexture() const;

    bool hasPreMultipliedAlpha() const { return m_pixelFormat.isPremultipliedAlpha(); }

    /// Convert this image to pre-multiplied alpha, assumes that the image is
    /// using post-multiplied pixel values
    void toPreMultipliedAlpha();
    /// Convert this image to post-multiplied alpha, assumes that the image is
    /// using pre-multiplied pixel values
    void toPostMultipliedAlpha();

  protected:

    /// Width of the image in pixels
    int m_width = 0;
    /// Height of the image in pixels
    int m_height = 0;
    /// Line size in bytes, or zero if the line size is calculated automatically
    int m_lineSize = 0;
    /// Pixel format of the image data
    PixelFormat m_pixelFormat;
    /// Pointer to the raw image data
    /// @todo change to QByteArray to get copy-on-write
    unsigned char* m_data = nullptr;
    /// If false, m_data is allocated by us - and will be deallocated by us as well.
    bool m_externalData = false;
    /// Generation count of the image used to indicate changes in the image
    /// data to determine when associated textures should be updated.
    size_t m_generation = 0;

  private:

    /// Get a texture associated with this image and create it if it does not
    /// exists already.
    Texture & getTexture() const;
    void updateTexture();

    mutable std::unique_ptr<Texture> m_texture;
    mutable Radiant::Mutex m_textureMutex;
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
    bool loadImage(QFile & file, const ImageInfo & info, int size);
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

#endif // LUMINOUS_OPENGLES
}

#endif
