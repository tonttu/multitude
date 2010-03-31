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

#ifndef LUMINOUS_IMAGE_CODEC_QT_HPP
#define LUMINOUS_IMAGE_CODEC_QT_HPP

#ifndef USE_QT45
#error "This code is only to be used with Qt version 4.5."
#endif

#include <Luminous/ImageCodec.hpp>


namespace Luminous
{

  /// Image codec to handle QT files
  class ImageCodecQT : public ImageCodec
  {
  public:
    ImageCodecQT(const char * suffix);
    virtual ~ImageCodecQT();
    virtual bool canRead(FILE * file);
    virtual std::string extensions() const;
    virtual std::string name() const;
    virtual bool ping(ImageInfo & image, FILE * file);
    virtual bool read(Image & image, FILE * file);
    virtual bool write(const Image & image, FILE * file);

  private:
    std::string m_suffix;
  };

}

#endif
