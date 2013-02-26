/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
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

#include "Export.hpp"
#include "ImageCodec.hpp"

class QSvgRenderer;

namespace Luminous {

class LUMINOUS_API ImageCodecSVG : public Luminous::ImageCodec
{
public:
  ImageCodecSVG();
  virtual ~ImageCodecSVG();
  virtual bool canRead(FILE * file) OVERRIDE;

  virtual QString extensions() const OVERRIDE;
  virtual QString name() const OVERRIDE;
  virtual bool ping(ImageInfo & info, FILE * file) OVERRIDE;
  virtual bool read(Image & image, FILE * file) OVERRIDE;
  /// not supported
  virtual bool write(const Image & image, FILE * file) OVERRIDE;
private:
  QSvgRenderer * updateSVG(FILE * file);
};

} // namespace Luminous

/// @endcond

#endif // IMAGECODECSVG_HPP
