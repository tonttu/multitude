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

#ifndef LUMINOUS_PIXELFORMAT_HPP
#define LUMINOUS_PIXELFORMAT_HPP

#include <Luminous/Export.hpp>
#include <Luminous/Luminous.hpp>

#include <string>

namespace Luminous
{
  /// Describes the pixel format of an image
  /** This class tells what type values there are in the pixels, and
      how they are aligned. It is implemented in a way that makes it
      easy to convert the pixel formats*/
  class LUMINOUS_API PixelFormat
  {
  public:

    /// Data type of a single channel
    enum ChannelType
    {
      TYPE_UNKNOWN,
      TYPE_BYTE         = GL_BYTE,
      TYPE_UBYTE        = GL_UNSIGNED_BYTE,
      TYPE_SHORT        = GL_SHORT,
      TYPE_USHORT       = GL_UNSIGNED_SHORT,
      TYPE_INT          = GL_INT,
      TYPE_UINT         = GL_UNSIGNED_INT,
      TYPE_FLOAT        = GL_FLOAT,
      TYPE_DOUBLE       = GL_DOUBLE
                        };
    /// Compression used
    enum Compression
    {
      COMPRESSION_NONE,                                                 ///< No compression
      COMPRESSED_RGB_DXT1         = GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    ///< DXT1 RGB compression
      COMPRESSED_RGBA_DXT1        = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   ///< DXT1 RGBA compression
      COMPRESSED_RGBA_DXT3        = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,   ///< DXT3 RGBA compression
      COMPRESSED_RGBA_DXT5        = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT    ///< DXT5 RGBA compression
    };

    /// Layout of channels
    enum ChannelLayout
    {
      LAYOUT_UNKNOWN,
      LAYOUT_COLOR_INDEX          = GL_COLOR_INDEX,
      LAYOUT_STENCIL_INDEX        = GL_STENCIL_INDEX,
      LAYOUT_DEPTH_COMPONENT      = GL_DEPTH_COMPONENT,
      LAYOUT_RED                  = GL_RED,
      LAYOUT_GREEN                = GL_GREEN,
      LAYOUT_BLUE                 = GL_BLUE,
      LAYOUT_ALPHA                = GL_ALPHA,
      LAYOUT_RGB                  = GL_RGB,
      LAYOUT_RGBA                 = GL_RGBA,
      LAYOUT_BGR                  = GL_BGR,
      LAYOUT_BGRA                 = GL_BGRA,
      LAYOUT_LUMINANCE            = GL_LUMINANCE,
      LAYOUT_LUMINANCE_ALPHA      = GL_LUMINANCE_ALPHA
                                  };

    /// Constructs a copy
    /// @param pf pixel format to copy
    PixelFormat(const PixelFormat& pf);
    /// Constructs a pixel format with the given info
    /// @param layout layout of the channels
    /// @param type channel data type
    PixelFormat(ChannelLayout layout = LAYOUT_UNKNOWN, ChannelType type = TYPE_UNKNOWN);
    /// Constructs a pixel format using compression
    /// @param compression compression to use
    PixelFormat(Compression compression);
    ~PixelFormat();

    /// Returns the number of channels
    int numChannels() const;
    /// Returns the layout of the channels
    ChannelLayout layout() const { return m_layout; }
    /// Returns the data type of the channels
    ChannelType type() const { return m_type; }
    /// Returns the compression method
    Compression compression() const { return m_compression; }
    /// Returns the number of bytes in a single pixel
    int bytesPerPixel() const;

    /// Constructs an 8-bit RGB pixel format
    /// @return new pixel format
    static PixelFormat rgbUByte()
    { return PixelFormat(LAYOUT_RGB, TYPE_UBYTE); }
    /// Constructs an 8-bit RGBA pixel format
    /// @return new pixel format
    static PixelFormat rgbaUByte()
    { return PixelFormat(LAYOUT_RGBA, TYPE_UBYTE); }

    /// Constructs an 8-bit BGR pixel format
    /// @return new pixel format
    static PixelFormat bgrUByte()
    { return PixelFormat(LAYOUT_BGR, TYPE_UBYTE); }
    /// Constructs an 8-bit BGRA pixel format    
    /// @return new pixel format
    static PixelFormat bgraUByte()
    { return PixelFormat(LAYOUT_BGRA, TYPE_UBYTE); }

    /// Constructs an 8-bit alpha-only pixel format
    /// @return new pixel format
    static PixelFormat alphaUByte()
    { return PixelFormat(LAYOUT_ALPHA, TYPE_UBYTE); }
    /// Constructs an 8-bit luminance (grayscale) pixel format
    /// @return new pixel format
    static PixelFormat luminanceUByte()
    { return PixelFormat(LAYOUT_LUMINANCE, TYPE_UBYTE); }
    /// Constructs a floating-point luminance pixel format
    /// @return new pixel format
    static PixelFormat luminanceFloat()
    { return PixelFormat(LAYOUT_LUMINANCE, TYPE_FLOAT); }
    /// Constructs a floating-point luminance-alpha pixel format
    /// @return new pixel format
    static PixelFormat luminanceAlphaFloat()
    { return PixelFormat(LAYOUT_LUMINANCE_ALPHA, TYPE_FLOAT); }
    /// Constructs an 8-bit luminance-alpha pixel format
    /// @return new pixel format
    static PixelFormat luminanceAlphaUByte()
    { return PixelFormat(LAYOUT_LUMINANCE_ALPHA, TYPE_UBYTE); }

    /// Compare if two pixel formats are the same
    inline bool operator == (const PixelFormat & that) const
    {
      return m_layout == that.m_layout && m_type == that.m_type &&
          m_compression == that.m_compression;
    }

    /// Compare if two pixel formats are not the same
    inline bool operator != (const PixelFormat & that) const
    {
      return !(*this == that);
    }

    /// Converts the pixel format into a human-readable string
    std::string toString() const;

  private:
    ChannelLayout m_layout;
    ChannelType m_type;
    Compression m_compression;
  };

}

#endif
