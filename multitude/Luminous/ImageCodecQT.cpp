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
  static bool checkFormat(QImage::Format formatIn, QImage::Format & qformatOut, PixelFormat & formatOut)
  {
    switch (formatIn) {
      case QImage::Format_Indexed8:
      case QImage::Format_Mono:
      case QImage::Format_MonoLSB:
        // Palette formats, we don't know the optimal pixel format without actually reading the image
        return false;

      case QImage::Format_Invalid:
      case QImage::NImageFormats:
        return false;

      case QImage::Format_Alpha8:
        qformatOut = QImage::Format_Alpha8;
        formatOut = PixelFormat::alphaUByte();
        break;

      case QImage::Format_Grayscale8:
      case QImage::Format_RGB32:
      case QImage::Format_RGB16:
      case QImage::Format_RGB666:
      case QImage::Format_RGB555:
      case QImage::Format_RGB888:
      case QImage::Format_RGB444:
      case QImage::Format_RGBX8888:
      case QImage::Format_RGBA8888:
      case QImage::Format_BGR30:
      case QImage::Format_RGB30:
      case QImage::Format_RGBX64:
        qformatOut = QImage::Format_RGB32;
        formatOut = PixelFormat::rgbUByte();
        break;

      case QImage::Format_ARGB32:
      case QImage::Format_ARGB32_Premultiplied:
      case QImage::Format_ARGB8565_Premultiplied:
      case QImage::Format_ARGB6666_Premultiplied:
      case QImage::Format_ARGB8555_Premultiplied:
      case QImage::Format_ARGB4444_Premultiplied:
      case QImage::Format_RGBA8888_Premultiplied:
      case QImage::Format_A2BGR30_Premultiplied:
      case QImage::Format_A2RGB30_Premultiplied:
      case QImage::Format_RGBA64:
      case QImage::Format_RGBA64_Premultiplied:
        qformatOut = QImage::Format_ARGB32;
        formatOut = PixelFormat::rgbaUByte();
        break;
    }
    return true;
  }

  static bool checkFormat(const QImage & img, QImage::Format & qformatOut, PixelFormat & formatOut)
  {
    if (img.format() == QImage::Format_Indexed8 ||
        img.format() == QImage::Format_Mono ||
        img.format() == QImage::Format_MonoLSB) {
      if (img.hasAlphaChannel()) {
        qformatOut = QImage::Format_ARGB32;
        formatOut = PixelFormat::rgbaUByte();
      } else {
        qformatOut = QImage::Format_RGB32;
        formatOut = PixelFormat::rgbUByte();
      }
      return true;
    }
    return checkFormat(img.format(), qformatOut, formatOut);
  }

  static bool load(QImage & img, QFile & file)
  {
    QImageReader r(&file);
    const bool findBiggestImage = r.format() == "ico";

    if (findBiggestImage) {
      QSize bestSize;
      int bestIndex = -1;

      for (int i = 0; i < r.imageCount(); ++i, r.jumpToNextImage()) {
        QSize size = r.size();
        PixelFormat pf;
        QImage::Format qf;
        bool ok = checkFormat(r.imageFormat(), qf, pf);
        if (!ok) {
          auto img = r.read();
          ok = checkFormat(img, qf, pf);
          size = img.size();
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
      QSize bestSize;
      PixelFormat bestPf;

      for (int i = 0; i < r.imageCount(); ++i, r.jumpToNextImage()) {
        QSize size = r.size();
        PixelFormat pf;
        QImage::Format qf;
        ok = checkFormat(r.imageFormat(), qf, pf);
        if (!ok) {
          auto img = r.read();
          ok = checkFormat(img, qf, pf);
          size = img.size();
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
      QSize size = r.size();
      PixelFormat pf;
      QImage::Format qf;
      ok = checkFormat(r.imageFormat(), qf, pf);
      /// Some ImageReader plugins like gif don't support image format scanning
      /// without actually reading the image
      if (!ok || size.isEmpty()) {
        QImage img = r.read();
        if (!img.isNull()) {
          ok = checkFormat(img, qf, pf);
          size = img.size();
        } else {
          ok = false;
        }
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

    QImage::Format dstFormat;
    PixelFormat format;
    if (!checkFormat(qi, dstFormat, format))
      return false;

    qi = std::move(qi).convertToFormat(dstFormat);
    if (qi.format() != dstFormat)
      return false;

    const int bytesPerPixel = qi.depth() / 8;
    image.allocate(qi.width(), qi.height(), format);

    const uint8_t * src = qi.bits();
    const uint8_t * sentinel = src + bytesPerPixel * qi.width() * qi.height();
    uint8_t * dest = image.data();

    if (dstFormat == QImage::Format_ARGB32) {
      while(src < sentinel) {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        dest[3] = src[3];
        src += bytesPerPixel;
        dest += 4;
      }
    } else if (dstFormat == QImage::Format_Grayscale8 || dstFormat == QImage::Format_Alpha8) {
      memcpy(dest, src, qi.width() * qi.height());
    } else if(dstFormat == QImage::Format_RGB32) {
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

  bool ImageCodecQT::write(const Image & image, QSaveFile & file)
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
    } else if(image.pixelFormat() == PixelFormat::bgrUByte()) {
      qi = QImage(image.width(), image.height(), QImage::Format_RGB32);

      uint8_t * dest = qi.bits();

      while(src < sentinel) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = 255;
        src += 3;
        dest += 4;
      }
    } else {
      Radiant::error("ImageCodecQT::write # Unsupported pixel format %s",
                     image.pixelFormat().toString().toUtf8().data());
    }

    // Radiant::info("File is almost written");
    return qi.save(&file, m_suffix.toUtf8().data());
  }

}
