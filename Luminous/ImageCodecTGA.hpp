/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef CODEC_TGA_HPP
#define CODEC_TGA_HPP

/// @cond

#include "Export.hpp"
#include "ImageCodec.hpp"

namespace Luminous
{

  /// Image codec to handle TGA files
  class LUMINOUS_API ImageCodecTGA : public ImageCodec
  {
    public:
      virtual ~ImageCodecTGA();
      virtual bool canRead(QFile & file);

      virtual QString extensions() const;
      virtual QString name() const;
      virtual bool ping(ImageInfo & info, QFile & file);
      virtual bool read(Image & image, QFile & file);
      virtual bool write(const Image & image, QSaveFile & file);
  };

}

/// @endcond

#endif
