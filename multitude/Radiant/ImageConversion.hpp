/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_IMAGE_CONVERSION_HPP
#define RADIANT_IMAGE_CONVERSION_HPP

#include "Export.hpp"

namespace Radiant {

  class VideoImage;

  /// VideoImage conversion utilities.
  /** This class contains static functions for converting between
      different image formats.

      New format conversion functions will be written as needed. */
  class RADIANT_API ImageConversion
  {
  public:
    /// Convert between image formats
    /// @param source source
    /// @param[out] target dest
    /// @return true on success
    static bool convert(const VideoImage * source, VideoImage * target);

    /// @copydoc YUV411PToRGB
    /// @deprecated not implemented
    static void YUV411ToRGB(const VideoImage * source, VideoImage * target);

    /// Convert image format
    /// @param source source image
    /// @param[out] target target image
    static void YUV411PToRGB(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void YUV411PToRGBA(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void YUV411ToGrayscale(const VideoImage * source, VideoImage * target);

    /// @copydoc YUV411PToRGB
    static void YUV420PToGrayscale(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void YUV420ToGrayscale(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    /// @deprecated not implemented
    static void YUV420ToRGBA(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void YUV420PToRGBA(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void YUV420PToRGB(const VideoImage * source, VideoImage * target);

    /// @copydoc YUV411PToRGB
    static void YUV422PToRGBA(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void YUV422PToGrayscale(const VideoImage * source, VideoImage * target);

    /// @copydoc YUV411PToRGB
    static void grayscaleToRGB(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void RGBToGrayscale(const VideoImage * source, VideoImage * target);

    /// @copydoc YUV411PToRGB
    static void bayerToRGB(const VideoImage * source, VideoImage * target);
    /// @copydoc YUV411PToRGB
    static void bayerToGrayscale(const VideoImage * source, VideoImage * target);
  };

}

#endif
