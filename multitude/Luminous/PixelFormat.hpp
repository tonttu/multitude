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
  /** This class tells what typy values there are in the pixels, and
      how they are aligned. It is implemented in away that makes it
      easy to convert the pixel formats*/
  /// @todo Doc
  class LUMINOUS_API PixelFormat
  {
    public:

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

      PixelFormat(const PixelFormat& pf);
      PixelFormat(ChannelLayout layout = LAYOUT_UNKNOWN, ChannelType type = TYPE_UNKNOWN);
      ~PixelFormat();

      int numChannels() const;
      ChannelLayout layout() const { return m_layout; }
      ChannelType type() const { return m_type; }
      int bytesPerPixel() const;

    static PixelFormat rgbUByte()
    { return PixelFormat(LAYOUT_RGB, TYPE_UBYTE); }
    static PixelFormat rgbaUByte()
    { return PixelFormat(LAYOUT_RGBA, TYPE_UBYTE); }
    static PixelFormat alphaUByte()
    { return PixelFormat(LAYOUT_ALPHA, TYPE_UBYTE); }
    static PixelFormat luminanceUByte()
    { return PixelFormat(LAYOUT_LUMINANCE, TYPE_UBYTE); }
    static PixelFormat luminanceFloat()
    { return PixelFormat(LAYOUT_LUMINANCE, TYPE_FLOAT); }

    inline bool operator == (const PixelFormat & that) const
    {
      return m_layout == that.m_layout && m_type == that.m_type;
    }

    std::string toString() const;

    protected:
      ChannelLayout m_layout;
      ChannelType m_type;
  };

}

#endif
