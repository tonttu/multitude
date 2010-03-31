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

// #include <Nimble/Vector2.hpp>

#include <cstdio>
#include <vector>

namespace Luminous
{
  class CodecRegistry;

  /// Simple struct containing basic image information that can be quickly
  /// queried (with Image::ping) without loading the full image.
  struct ImageInfo {
    ImageInfo() : width(0), height(0) {}
    int width;
    int height;
    PixelFormat pf;
  };

  /// Simple image handling class
  /** This class is used mainly to load/save image data in various formats.
      It provides minimal image processing support (resizing).

      The image data is stored in a single, straightforward buffer.
  */
  /// @todo Split this into two classes, so that "Image" does not inherit ContextVariableT
  class LUMINOUS_API Image :
      public Luminous::ContextVariableT<Luminous::Texture2D>
  {
  public:
    Image();
    Image(const Image& img);
    virtual ~Image();

    /// Allocates memory, for an image of given size and format
    void allocate(int width, int height, const PixelFormat & pf);

    /// Returns the aspect ration of the image (if defined)
    float aspect() const { return (float)m_width / (float)m_height; }

    /// The width ofthe image in pixels
    int width() const { return m_width; }
    /// The height ofthe image in pixels
    int height() const { return m_height; }
    /// The size of the image in pixels
    Nimble::Vector2i size() const
    { return Nimble::Vector2i(m_width, m_height); }
    /// The number of bytes a single line in the image takes
    int lineSize() { return m_width * m_pixelFormat.numChannels(); }
    /// Returns a pointer to a specific line
    unsigned char* line(unsigned y) { return &m_data[y * lineSize()]; }
    /// Returns a pointer to the image data
    unsigned char * bytes() { return & m_data[0]; }
    /// Returns a const pointer to the image data
    const unsigned char * bytes() const { return & m_data[0]; }

    /// Returns a pointer to the image data
    unsigned char * data() { return & m_data[0]; }
    /// Returns a const pointer to the image data
    const unsigned char * data() const { return & m_data[0]; }

    /// Check if a file is readable, and returns its core information
    static bool ping(const char * filename, ImageInfo & info);

    /** Read a file to this Image object. */
    bool read(const char * filename);
    /** Write this Image to a file. */
    bool write(const char * filename);

    /** Create an image object from data provided by the user. */
    void fromData(const unsigned char * bytes, int width, int height,
          PixelFormat format);

    const PixelFormat& pixelFormat() const { return m_pixelFormat; }

    /// Clears the image, freeing the data.
    void clear();

    /// Returns true if the image does not contain any data.
    bool empty() const { return (m_data == 0); }

    /// Flip the image upside down
    void flipVertical();

    /** Resample a source image using straightforward bilinear
    interpolation. */
    bool copyResample(const Image & source, int w, int h);

    /** Down-sample the image to quarter size. */
    bool quarterSize(const Image & source);
    /** Remove pixels from the right edge of the image. */
    bool forgetLastPixels(int n);
    /** Remove lines from the bottom of the image. */
    void forgetLastLines(int n);
    /** Removes the last line from the image. */
    void forgetLastLine();
    /** Checks that the image dimensions are efasible for a texture. In practice
        this functions removes pixels from the right and bottom, to make the
        width and height mutiples of two.
    */
    void makeValidTexture();
    /** Returns true if the image has an alpha channel. */
    bool hasAlpha() const;
    /** Copies the argument image to this image. */
    Image & operator = (const Image& img);

    /** Returns a pointer to the file-format codecs. */
    static CodecRegistry * codecs();

    /** Binds a texture representing this image to the current OpenGL context.

        @arg textureUnit The OpenGL texture unit to bind to
        @arg withmipmaps Should we use mimaps, or not. This argument only
        makes difference the first time this function executed for the context
        (and the texture is created), after that the the same texture is used.
    */
    void bind(GLenum textureUnit = GL_TEXTURE0, bool withmimaps = true);

  protected:

    void sample(float x1, float y1, float x2, float y2, Image & dest, int destX, int destY) const;
    float computeWeight(int x, int y, float x1, float y1, float x2, float y2) const;

  private:

    int m_width;
    int m_height;
    PixelFormat m_pixelFormat;
    unsigned char* m_data;
  };

}

#endif
