/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef IMAGECODECCS_HPP
#define IMAGECODECCS_HPP

#include "ImageCodec.hpp"
#include "PixelFormat.hpp"

#include <Nimble/Vector2.hpp>

#include <vector>

namespace Luminous {


class ImageCodecCS : public ImageCodec
{
public:
  bool canRead(QFile & file);
  QString extensions() const;
  QString name() const;
  bool ping(ImageInfo & info, QFile & file);
  bool read(Image & image, QFile & file);
  bool write(const Image & image, QFile & file);
};

}

#endif // IMAGECODECCS_HPP
