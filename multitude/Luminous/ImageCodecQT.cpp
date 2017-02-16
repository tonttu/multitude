/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ImageCodecQT.hpp"

#include "Image.hpp"

#include <Radiant/Trace.hpp>

#include <QFile>
#include <QImageReader>
#include <QImage>

#include <cstdint>

namespace Luminous
{
  static bool checkFormat(QImage::Format formatIn, QSize sizeIn,
                          PixelFormat & formatOut, Nimble::Size & sizeOut)
  {
    switch (formatIn) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_RGB32:
    case QImage::Format_RGB16:
    case QImage::Format_RGB666:
    case QImage::Format_RGB555:
    case QImage::Format_RGB888:
    case QImage::Format_RGB444:
      formatOut = PixelFormat::rgbUByte();
      break;
    /// @todo Indexed8 might have a alpha channel, we don't know without actually opening the image.
    ///       Should we open it?
    case QImage::Format_Indexed8:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
      formatOut = PixelFormat::rgbaUByte();
      break;
    case QImage::Format_Grayscale8:
      formatOut = PixelFormat::redUByte();
      break;
    default:
      return false;
    }
    sizeOut.make(sizeIn.width(), sizeIn.height());
    return sizeOut.isValid();
  }

  static bool load(QImage & img, QFile & file)
  {
    QImageReader r(&file);
    const bool findBiggestImage = r.format() == "ico";

    if (findBiggestImage) {
      Nimble::Size bestSize;
      int bestIndex = -1;

      for (int i = 0; i < r.imageCount(); ++i, r.jumpToNextImage()) {
        Nimble::Size size;
        PixelFormat pf;
        bool ok = checkFormat(r.imageFormat(), r.size(), pf, size);
        if (!ok) {
          auto img = r.read();
          ok = checkFormat(img.format(), img.size(), pf, size);
        }
        if (ok && (!bestSize.isValid() || (size.width()*size.height() > bestSize.width()*bestSize.height()))) {
          bestSize = size;
          bestIndex = i;
        }
      }

      if (bestIndex >= 0) {
        r.jumpToImage(bestIndex);
      }
    }
    return r.read(&img);
  }

  ImageCodecQT::ImageCodecQT(const char * suffix)
    : m_suffix(suffix)
  {}

  ImageCodecQT::~ImageCodecQT()
  {}

  bool ImageCodecQT::canRead(QFile & file)
  {
    QImageReader r(&file);
    return r.canRead();
  }

  QString ImageCodecQT::extensions() const
  {
    return m_suffix;
  }

  QString ImageCodecQT::name() const
  {
    return QString("ImageCodecQT");
  }

  bool ImageCodecQT::ping(ImageInfo & info, QFile & file)
  {
    QImageReader r(&file);

    if(!r.canRead()) {
      Radiant::error("ImageCodecQT::ping # no valid data or the file format is not supported");
      return false;
    }

    const bool findBiggestImage = r.format() == "ico";

    bool ok = false;
    if (findBiggestImage) {
      Nimble::Size bestSize;
      PixelFormat bestPf;

      for (int i = 0; i < r.imageCount(); ++i, r.jumpToNextImage()) {
        Nimble::Size size;
        PixelFormat pf;
        ok = checkFormat(r.imageFormat(), r.size(), pf, size);
        if (!ok) {
          auto img = r.read();
          ok = checkFormat(img.format(), img.size(), pf, size);
        }
        if (ok && (!bestSize.isValid() || (size.width()*size.height() > bestSize.width()*bestSize.height()))) {
          bestSize = size;
          bestPf = pf;
        }
      }

      if (bestSize.isValid()) {
        ok = true;
        info.pf = bestPf;
        info.width = bestSize.width();
        info.height = bestSize.height();
      } else {
        ok = false;
      }
    } else {
      Nimble::Size size;
      PixelFormat pf;
      /// Some ImageReader plugins like gif don't support image format scanning
      /// without actually reading the image
      if (r.imageFormat() == QImage::Format_Invalid && !r.size().isEmpty()) {
        QImage img = r.read();
        if (!img.isNull()) {
          ok = checkFormat(img.format(), img.size(), pf, size);
        }
      } else {
        ok = checkFormat(r.imageFormat(), r.size(), pf, size);
      }
      if (ok) {
        info.pf = pf;
        info.width = size.width();
        info.height = size.height();
      } else {
        Radiant::error("ImageCodecQT::ping # image has unsupported pixel format (%d: %dx%d)",
                       r.imageFormat(), r.size().width(), r.size().height());
      }
    }
    return ok;
  }

  bool ImageCodecQT::read(Image & image, QFile & file)
  {
    QImage qi;
    if (!load(qi, file))
      return false;

    QImage::Format fmt = qi.format();

    if ((fmt != QImage::Format_ARGB32 && fmt != QImage::Format_RGB32) ||
        (qi.depth() != 32 && qi.depth() != 24 && qi.depth() != 8)) {
      qi = qi.convertToFormat(qi.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
      fmt = qi.format();
    }

    if ((fmt != QImage::Format_ARGB32 && fmt != QImage::Format_RGB32) ||
        (qi.depth() != 32 && qi.depth() != 24 && qi.depth() != 8))
      return false;

    const int bytesPerPixel = qi.depth() / 8;
    image.allocate(qi.width(), qi.height(), fmt == QImage::Format_ARGB32 ? PixelFormat::rgbaUByte() : PixelFormat::rgbUByte());

    const uint8_t * src = qi.bits();
    const uint8_t * sentinel = src + bytesPerPixel * qi.width() * qi.height();
    uint8_t * dest = image.data();

    if(fmt == QImage::Format_ARGB32) {
      if (bytesPerPixel != 1) {
        while(src < sentinel) {
          dest[0] = src[2];
          dest[1] = src[1];
          dest[2] = src[0];
          dest[3] = src[3];
          src += bytesPerPixel;
          dest += 4;
        }
      } else {
        while(src < sentinel) {
          dest[0] = src[0];
          dest[1] = src[0];
          dest[2] = src[0];
          dest[3] = 255;
          src++;
          dest += 4;
        }
      }
    }
    else if(fmt == QImage::Format_RGB32) {
      while(src < sentinel) {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        src += bytesPerPixel;
        dest += 3;
      }
    }

    return true;
  }

  bool ImageCodecQT::write(const Image & image, QFile & file)
  {
    QImage qi;

    const uint8_t *src = image.data();
    const uint8_t *sentinel = src +
                              image.pixelFormat().bytesPerPixel() * image.width() *
                              image.height();

    if(image.pixelFormat() == PixelFormat::rgbUByte()) {

      // Radiant::info("File is almost written rbgUByte");

      qi = QImage(image.width(), image.height(),
                  QImage::Format_RGB32);

      uint8_t * dest = qi.bits();

      while(src < sentinel) {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        dest[3] = 255;
        src += 3;
        dest += 4;
      }
    }
    else if(image.pixelFormat() == PixelFormat::rgbaUByte()) {
      // Radiant::info("File is almost written rbgaUByte");

      qi = QImage(image.width(), image.height(),
                  QImage::Format_ARGB32);

      uint8_t * dest = qi.bits();

      while(src < sentinel) {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        dest[3] = src[3];
        src += 4;
        dest += 4;
      }
    }
    else if(image.pixelFormat() == PixelFormat::redUByte()) {

      qi = QImage(image.width(), image.height(),
                  QImage::Format_RGB32);

      uint8_t * dest = qi.bits();

      while(src < sentinel) {
        dest[0] = src[0];
        dest[1] = src[0];
        dest[2] = src[0];
        dest[3] = 255;
        src ++;
        dest += 4;
      }
    } else if(image.pixelFormat() == PixelFormat::bgraUByte()) {
      qi = QImage(image.width(), image.height(), QImage::Format_ARGB32);

      uint8_t * dest = qi.bits();

      while(src < sentinel) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 4;
        dest += 4;
      }
    } else {
      Radiant::error("ImageCodecQT::write # Unsupported pixel format");
    }

    // Radiant::info("File is almost written");
    return qi.save(&file, m_suffix.toUtf8().data());
  }

}
