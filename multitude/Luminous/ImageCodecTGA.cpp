/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ImageCodecTGA.hpp"
#include "PixelFormat.hpp"
#include "Image.hpp"

#include <Radiant/Trace.hpp>

#include <string.h>

namespace Luminous
{

  typedef struct
  {
    unsigned char identSize;
    unsigned char colormapType;
    unsigned char imageType;

    unsigned char ignored[5];

    unsigned char xStartLo;
    unsigned char xStartHi;
    unsigned char yStartLo;
    unsigned char yStartHi;

    unsigned char widthLo;
    unsigned char widthHi;
    unsigned char heightLo;
    unsigned char heightHi;
    unsigned char bpp;
    unsigned char descriptor; // 0x00vhaaaa (v = vertical flip, h = horizontal flip, a = alpha)
  } TGAHeader;


  ImageCodecTGA::~ImageCodecTGA()
  {}

  bool ImageCodecTGA::canRead(QFile & file)
  {
    TGAHeader header;

    long pos = file.pos();

    auto bytesRead = file.read(reinterpret_cast<char *>(&header), sizeof(TGAHeader));
    file.seek(pos);
    if(bytesRead != sizeof(TGAHeader)) return false;

    int w = header.widthLo + (header.widthHi << 8);
    int h = header.heightLo + (header.heightHi << 8);
    int bpp = header.bpp;


    // Check image type
    bool ok = false;

    switch(header.imageType) {
      case 2:   // RGB
      case 3:   // Grayscale
      case 10:  // RGB+RLE
      case 11:  // Grayscale+RLE
        ok = true;
    };

    if(header.colormapType != 0) ok = false;

    return ok && (w > 0) && (h > 0) && ((bpp == 8) || (bpp == 24) || (bpp == 32));
  }

  QString ImageCodecTGA::extensions() const
  {
    return QString("tga");
  }

  QString ImageCodecTGA::name() const
  {
    return QString("targa");
  }

  bool ImageCodecTGA::ping(ImageInfo & info, QFile & file)
  {
    TGAHeader header;
    auto pos = file.pos();
    auto bytesRead = file.read(reinterpret_cast<char *>(&header), sizeof(TGAHeader));
    file.seek(pos);
    if(bytesRead != sizeof(TGAHeader)) return false;

    info.width = header.widthLo + (header.widthHi << 8);
    info.height = header.heightLo + (header.heightHi << 8);
    int bytesPerPixel = (header.bpp >> 3);

    switch(bytesPerPixel) {
      case 4:
        info.pf = PixelFormat(PixelFormat::LAYOUT_BGRA, PixelFormat::TYPE_UBYTE);
        break;
      case 3:
        info.pf = PixelFormat(PixelFormat::LAYOUT_BGR, PixelFormat::TYPE_UBYTE);
        break;
      case 1:
        info.pf = PixelFormat(PixelFormat::LAYOUT_RED, PixelFormat::TYPE_UBYTE);
        break;
      default:
        Radiant::error("Image::readTGAHeader # unsupported bit depth (%d)", header.bpp);
        return false;
    };
    return true;
  }

  bool ImageCodecTGA::read(Image & image, QFile & file)
  {
    TGAHeader header;
    // Read header
    auto bytesRead = file.read(reinterpret_cast<char *>(&header), sizeof(TGAHeader));
    if (bytesRead != sizeof(TGAHeader)) {
      Radiant::error("ImageCodecTGA::read # failed to read image header");
      return false;
    }

    // Check image type
    bool typeGood = false;

    switch(header.imageType) {
      case 2:   // RGB
      case 3:   // Grayscale
      case 10:  // RGB+RLE
      case 11:  // Grayscale+RLE
        typeGood = true;
    };

    const int width = header.widthLo + (header.widthHi << 8);
    const int height = header.heightLo + (header.heightHi << 8);
    const int bytesPerPixel = (header.bpp >> 3);

    if(!typeGood || width == 0 || height == 0)
      return false;

    // Choose pixel format
    PixelFormat pf;
    switch(bytesPerPixel) {
      case 4:
        pf = PixelFormat(PixelFormat::LAYOUT_BGRA, PixelFormat::TYPE_UBYTE);
        break;
      case 3:
        pf = PixelFormat(PixelFormat::LAYOUT_BGR, PixelFormat::TYPE_UBYTE);
        break;
      case 1:
        pf = PixelFormat(PixelFormat::LAYOUT_RED, PixelFormat::TYPE_UBYTE);
        break;
      default:
        Radiant::error("ImageCodecTGA::read # unsupported bit depth (%d)", header.bpp);
        return false;
    };

    // Radiant::trace(Radiant::INFO, "W %d H %d", width, height);

    // Allocate memory
    image.allocate(width, height, pf);

    unsigned int size = (unsigned) (width * height * bytesPerPixel);

    // Skip the ident field if present
    if(header.identSize > 0)
      file.seek( file.pos() + header.identSize);

    if(header.imageType == 2 || header.imageType == 3) {

      // Uncompressed image
      auto bytesRead = file.read(reinterpret_cast<char *>(image.bytes()), size);
      if (bytesRead != size) {
        Radiant::error("ImageCodecTGA::read # failed to read uncompressed pixel data");
        return false;
      }

    } else {

      // Compressed image
      int pixels = width * height;
      int currentPixel = 0;
      size_t currentByte = 0;
      unsigned char * pixel = new unsigned char [bytesPerPixel];

      do {
        unsigned char chunkHeader = 0;
        auto bytesRead = file.read(reinterpret_cast<char *>(&chunkHeader), 1);
        if (bytesRead != 1) {
          Radiant::error("ImageCodecTGA::read # failed to read compressed chunk header");
          delete[] pixel;
          return false;
        }

        if(chunkHeader < 128) {
          chunkHeader++;

          for(int i = 0; i < chunkHeader; i++) {
            unsigned char * dst = image.bytes() + currentByte;
            auto bytesRead = file.read(reinterpret_cast<char *>(dst), bytesPerPixel);
            if(bytesRead != bytesPerPixel) {
              Radiant::error("ImageCodecTGA::read # failed to read pixel data");
              return false;
            }

            currentByte += bytesPerPixel;
            currentPixel++;
          }
        } else {
          chunkHeader -= 127;
          auto bytesRead = file.read(reinterpret_cast<char *>(pixel), bytesPerPixel);
          if(bytesRead != bytesPerPixel) {
            Radiant::error("ImageCodecTGA::read # failed to read pixel data");
            delete[] pixel;
            return false;
          }

          for(int i = 0; i < chunkHeader; i++) {
            unsigned char * dst = image.bytes() + currentByte;
            memcpy(dst, pixel, bytesPerPixel);

            currentByte += bytesPerPixel;
            currentPixel++;
          }
        }
      } while(currentPixel < pixels);

      delete[] pixel;
    }

    // Check flip bit
//    if(header.descriptor & (1 << 5))
//      flipVertical();

    return true;

  }

  bool ImageCodecTGA::write(const Image & image, QSaveFile & file)
  {
    // Fill the header
    TGAHeader header;
    memset(&header, 0, sizeof(TGAHeader));

    if(image.pixelFormat().type() != PixelFormat::TYPE_UBYTE) {
      Radiant::error("Image::writeTGA # can only write images with pixel data type UBYTE");
      return false;
    }

    bool reverse = false;

    switch(image.pixelFormat().layout()) {
      case PixelFormat::LAYOUT_RGB:
        reverse = true;
      case PixelFormat::LAYOUT_BGR:
        header.imageType = 2;
        break;
      case PixelFormat::LAYOUT_RGBA:
        reverse = true;
      case PixelFormat::LAYOUT_BGRA:
        header.imageType = 2;
        break;
      case PixelFormat::LAYOUT_RED:
      case PixelFormat::LAYOUT_ALPHA:
        header.imageType = 3;
        break;
      default:
        Radiant::error("Image::writeTGA # unsupported pixel layout");
        return false;
    };

    header.widthLo = static_cast<unsigned char> (image.width() & 0x00FF);
    header.widthHi = static_cast<unsigned char> ((image.width() >> 8) & 0xFF);
    header.heightLo = static_cast<unsigned char> (image.height() & 0x00FF);
    header.heightHi = static_cast<unsigned char> ((image.height() >> 8) & 0xFF);
    header.bpp = static_cast<unsigned char> (image.pixelFormat().numChannels() << 3);
    /// flip y
    header.descriptor |= (1 << 5);

    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(TGAHeader));

    // Write data
    if(!reverse) {
      int size = image.width() * image.height() * image.pixelFormat().numChannels();
      file.write(reinterpret_cast<const char*>(image.bytes()), size);
    } else {

      int pixels = image.width() * image.height();
      int bytesPerPixel = image.pixelFormat().numChannels();
      int currentPixel = 0;
      int currentByte = 0;

      unsigned char * pixel = new unsigned char [bytesPerPixel];

      do {
        memcpy(pixel, &image.bytes()[currentByte], bytesPerPixel);

        pixel[0] = image.bytes()[currentByte + 2];
        pixel[2] = image.bytes()[currentByte + 0];

        file.write(reinterpret_cast<const char*>(pixel), bytesPerPixel);

        currentPixel++;
        currentByte += bytesPerPixel;

      } while(currentPixel < pixels);

      delete [] pixel;
    }

    return true;
  }

}
