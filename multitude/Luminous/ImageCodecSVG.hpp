/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
  virtual bool canRead(QFile & file) OVERRIDE;

  virtual QString extensions() const OVERRIDE;
  virtual QString name() const OVERRIDE;
  virtual bool ping(ImageInfo & info, QFile & file) OVERRIDE;
  virtual bool read(Image & image, QFile & file) OVERRIDE;
  /// not supported
  virtual bool write(const Image & image, QFile & file) OVERRIDE;
};

} // namespace Luminous

/// @endcond

#endif // IMAGECODECSVG_HPP
