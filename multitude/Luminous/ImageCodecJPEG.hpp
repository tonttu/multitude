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

#ifndef LUMINOUS_IMAGE_CODEC_JPEG_HPP
#define LUMINOUS_IMAGE_CODEC_JPEG_HPP

#include <Luminous/ImageCodec.hpp>

namespace Luminous
{

  /// Image codec to handle JPEG files
  class ImageCodecJPEG : public ImageCodec
  {
    public:
      virtual ~ImageCodecJPEG();
      virtual bool canRead(FILE * file);
      virtual std::string extensions() const;
      virtual std::string name() const;
      virtual bool ping(ImageInfo & image, FILE * file);
      virtual bool read(Image & image, FILE * file);
      virtual bool write(const Image & image, FILE * file);  
  };

}

#endif
