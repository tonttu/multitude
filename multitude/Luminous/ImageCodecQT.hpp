/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_IMAGE_CODEC_QT_HPP
#define LUMINOUS_IMAGE_CODEC_QT_HPP

/// @cond

#include "Export.hpp"
#include "ImageCodec.hpp"

namespace Luminous
{

  /// Image codec to handle QT files
  class LUMINOUS_API ImageCodecQT : public ImageCodec
  {
  public:
    ImageCodecQT(const char * suffix);
    virtual ~ImageCodecQT();
    virtual bool canRead(QFile & file) OVERRIDE;
    virtual QString extensions() const OVERRIDE;
    virtual QString name() const OVERRIDE;
    virtual bool ping(ImageInfo & image, QFile & file) OVERRIDE;
    virtual bool read(Image & image, QFile & file) OVERRIDE;
    virtual bool write(const Image & image, QSaveFile & file) OVERRIDE;

  private:
    QString m_suffix;
  };

}

/// @endcond

#endif
