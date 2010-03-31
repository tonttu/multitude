#include "ImageCodecQT.hpp"

#include "Image.hpp"

#include <Radiant/Trace.hpp>

#include <QFile>
#include <QImageReader>
#include <QImage>

#include <stdint.h>

namespace Luminous
{
  using namespace Radiant;

  ImageCodecQT::ImageCodecQT(const char * suffix)
      : m_suffix(suffix)
  {}

  ImageCodecQT::~ImageCodecQT()
  {}

  bool ImageCodecQT::canRead(FILE * file)
  {
    QFile f;
    f.open(file, QIODevice::ReadOnly);
    QImageReader r(&f);
    return r.canRead();
  }

  std::string ImageCodecQT::extensions() const
  {
    return m_suffix;
  }

  std::string ImageCodecQT::name() const
  {
    return std::string("ImageCodecQT");
  }

  bool ImageCodecQT::ping(ImageInfo & info, FILE * file)
  {
    QFile f;
    if(!f.open(file, QIODevice::ReadOnly)) {	
		Radiant::error("ImageCodecQT::ping # failed to open file");
      return false;
	}
	
    QImageReader r(&f);

    if(!r.canRead()) {
		Radiant::error("ImageCodecQT::ping # no valid data or the file format is not supported");
      return false;
	}

    QImage::Format fmt = r.imageFormat();

    if(fmt == QImage::Format_RGB32)
      info.pf = PixelFormat::rgbUByte();
    else if(fmt == QImage::Format_ARGB32)
      info.pf = PixelFormat::rgbaUByte();
    else {
		Radiant::error("ImageCodecQT::ping # image has unsupported pixel format");
      return false;
	}

    QSize s = r.size();
    info.width = s.width();
    info.height = s.height();

    return true;
  }

  bool ImageCodecQT::read(Image & image, FILE * file)
  {
    QFile f;
    if(!f.open(file, QIODevice::ReadOnly))
      return false;
    QImageReader r(&f);

    if(!r.canRead())
      return false;

    QImage::Format fmt = r.imageFormat();
    PixelFormat pf;
    if(fmt == QImage::Format_RGB32)
      pf = PixelFormat::rgbUByte();
    else if(fmt == QImage::Format_ARGB32)
      pf = PixelFormat::rgbaUByte();
    else
      return false;

    QImage qi;
    if(!r.read(&qi))
      return false;

    image.allocate(qi.width(), qi.height(), pf);

    const uint8_t * src = qi.bits();
    const uint8_t * sentinel = src + 4 * qi.width() * qi.height();
    uint8_t * dest = image.data();

    if(fmt == QImage::Format_ARGB32) {
      while(src < sentinel) {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        dest[3] = src[3];
        src += 4;
        dest += 4;
      }
    }
    else if(fmt == QImage::Format_RGB32) {
      while(src < sentinel) {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        src += 4;
        dest += 3;
      }
    }

    return true;
  }

  bool ImageCodecQT::write(const Image & image, FILE * file)
  {
    QImage qi;

    const uint8_t *src = image.data();
    const uint8_t *sentinel = src +
        image.pixelFormat().bytesPerPixel() * image.width() *
        image.height();

    if(image.pixelFormat() == PixelFormat::rgbUByte()) {

      // info("File is almost written rbgUByte");

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
      // info("File is almost written rbgaUByte");

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
    else if(image.pixelFormat() == PixelFormat::luminanceUByte()) {

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
    }
    else {
      error("ImageCodecQT::write # Unsupported pixel format");
    }

    // info("File is almost written");

    QFile f;
    bool ok = f.open(file, QIODevice::ReadWrite);

    if(!ok) {
      // error("Error in QFile.open()");
      return false;
    }

    return qi.save(&f, m_suffix.c_str());
  }

}
