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

#ifndef IMAGECODECSVG_HPP
#define IMAGECODECSVG_HPP

/// @cond

#include <Luminous/ImageCodec.hpp>

class QSvgRenderer;

namespace Luminous {

class ImageCodecSVG : public Luminous::ImageCodec
{
public:
  ImageCodecSVG();
  virtual ~ImageCodecSVG();
  virtual bool canRead(FILE * file);

  virtual std::string extensions() const;
  virtual std::string name() const;
  virtual bool ping(ImageInfo & info, FILE * file);
  virtual bool read(Image & image, FILE * file);
  /// not supported
  virtual bool write(const Image & image, FILE * file);
private:
  QSvgRenderer * updateSVG(FILE * file);
};

} // namespace Luminous

/// @endcond

#endif // IMAGECODECSVG_HPP
