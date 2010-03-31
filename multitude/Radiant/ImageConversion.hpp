/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_IMAGE_CONVERSION_HPP
#define RADIANT_IMAGE_CONVERSION_HPP

#include <Radiant/Export.hpp>

namespace Radiant {

  class VideoImage;

  /// VideoImage conversion utilities.
  /** This class contains static functions for converting between
      different image formats.

      New format conversion functions will be written as needed. */

  class RADIANT_API ImageConversion
  {
  public:
    /// Convert from a random format to another random format
    static bool convert(const VideoImage * source, VideoImage * target);

    static void YUV411ToRGB(const VideoImage * source, VideoImage * target);
    static void YUV411PToRGB(const VideoImage * source, VideoImage * target);
    static void YUV411PToRGBA(const VideoImage * source, VideoImage * target);
    static void YUV411ToGrayscale(const VideoImage * source, VideoImage * target);

    static void YUV420PToGrayscale(const VideoImage * source, VideoImage * target);
    static void YUV420ToGrayscale(const VideoImage * source, VideoImage * target);
    static void YUV420ToRGBA(const VideoImage * source, VideoImage * target);
    static void YUV420PToRGBA(const VideoImage * source, VideoImage * target);
    static void YUV420PToRGB(const VideoImage * source, VideoImage * target);

    static void YUV422PToRGBA(const VideoImage * source, VideoImage * target);
    static void YUV422PToGrayscale(const VideoImage * source, VideoImage * target);

    static void grayscaleToRGB(const VideoImage * source, VideoImage * target);
    static void RGBToGrayscale(const VideoImage * source, VideoImage * target);

    static void bayerToRGB(const VideoImage * source, VideoImage * target);
    static void bayerToGrayscale(const VideoImage * source, VideoImage * target);
  };

}

#endif
