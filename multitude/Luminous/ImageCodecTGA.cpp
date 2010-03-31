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

  bool ImageCodecTGA::canRead(FILE * file)
  {    
    TGAHeader header;

    long pos = ftell(file);

    size_t r = fread(&header, sizeof(TGAHeader), 1, file);
    if(r != 1) return false;

    int w = header.widthLo + (header.widthHi << 8);
    int h = header.heightLo + (header.heightHi << 8);
    int bpp = header.bpp;

    fseek(file, pos, SEEK_SET);

    return (w > 0) && (h > 0) && ((bpp == 8) || (bpp == 24) || (bpp == 32));    
  }

  std::string ImageCodecTGA::extensions() const
  {
    return std::string("tga");
  }

  std::string ImageCodecTGA::name() const
  {
    return std::string("targa");
  }
  
  bool ImageCodecTGA::ping(ImageInfo & info, FILE * file)
  {
    TGAHeader header;

    size_t r = fread(&header, sizeof(TGAHeader), 1, file);
    if(r != 1) return false;

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
        info.pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE);
        break;
      default:
        Radiant::error("Image::readTGAHeader # unsupported bit depth (%d)", header.bpp);
        return false;
    };

    return true;
  }

  bool ImageCodecTGA::read(Image & image, FILE * file) 
  {
    TGAHeader header;

    // Read header
    size_t r = fread(&header, sizeof(TGAHeader), 1, file);
    if(r != 1) {
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

    int width = header.widthLo + (header.widthHi << 8);
    int height = header.heightLo + (header.heightHi << 8);
    size_t bytesPerPixel = (header.bpp >> 3);

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
        pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE);
        break;
      default:
        Radiant::error("ImageCodecTGA::read # unsupported bit depth (%d)", header.bpp);
        return false;
    };

    Radiant::trace(Radiant::INFO, "W %d H %d", width, height);

    // Allocate memory
    image.allocate(width, height, pf);

    unsigned int size = width * height * bytesPerPixel;

    // Skip the ident field if present
    if(header.identSize > 0) 
      fseek(file, header.identSize, SEEK_CUR);

    if(header.imageType == 2 || header.imageType == 3) {

      // Uncompressed image
      if(fread(image.bytes(), size, 1, file) != 1) {
        Radiant::error("ImageCodecTGA::read # failed to read uncompressed pixel data");
        return false;
      }

    } else {

      // Compressed image
      int pixels = width * height;
      int currentPixel = 0;
      int currentByte = 0;
      unsigned char * pixel = new unsigned char [bytesPerPixel];      

      do {
        unsigned char chunkHeader = 0;
        if(fread(&chunkHeader, 1, 1, file) != 1) {
          Radiant::error("ImageCodecTGA::read # failed to read compressed chunk header");
          return false;
        }

        if(chunkHeader < 128) {
          chunkHeader++;

          for(int i = 0; i < chunkHeader; i++) {
            unsigned char * dst = image.bytes() + currentByte;
            if(fread(dst, bytesPerPixel, 1, file) != 1) {
              Radiant::error("ImageCodecTGA::read # failed to read pixel data");
              return false;
            }

            currentByte += bytesPerPixel;
            currentPixel++;
          }
        } else {
          chunkHeader -= 127;

          if(fread(pixel, 1, bytesPerPixel, file) != bytesPerPixel) {
            Radiant::error("ImageCodecTGA::read # failed to read pixel data");
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

  bool ImageCodecTGA::write(const Image & image, FILE * file)
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
      case PixelFormat::LAYOUT_LUMINANCE:
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

    // Write header
    fwrite(&header, sizeof(TGAHeader), 1, file);

    // Write data
    if(!reverse) {
      int size = image.width() * image.height() * image.pixelFormat().numChannels();
      fwrite(image.bytes(), 1, size, file);
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

        fwrite(pixel, 1, bytesPerPixel, file);

        currentPixel++;
        currentByte += bytesPerPixel;

      } while(currentPixel < pixels);

      delete [] pixel;
    }

    return true;
  }

}
