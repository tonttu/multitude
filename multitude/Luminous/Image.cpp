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

#include "Image.hpp"
#include "ImageCodec.hpp"
#include "Luminous.hpp"
#include "CodecRegistry.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/Trace.hpp>

#include <algorithm>
#include <cctype>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <typeinfo>

#ifdef WIN32
#include <strings.h>	// strcasecmp()
#endif

using namespace std;
using namespace Radiant;

namespace Luminous
{

  CodecRegistry * Image::codecs()
  {
    static CodecRegistry cr;

    return &cr;
  }

  Image::Image()
      : m_width(0),
      m_height(0),
      m_pixelFormat(PixelFormat::LAYOUT_UNKNOWN, PixelFormat::TYPE_UNKNOWN),
      m_data(0)
  {}

  Image::Image(const Image& img)
      : m_width(0),
      m_height(0),
      m_pixelFormat(PixelFormat::LAYOUT_UNKNOWN, PixelFormat::TYPE_UNKNOWN),
      m_data(0)
  {
    allocate(img.m_width, img.m_height, img.m_pixelFormat);

    unsigned int bytes = m_width * m_height * m_pixelFormat.numChannels();
    memcpy(m_data, img.m_data, bytes);
  }

  Image::~Image()
  {
    clear();
  }

  void Image::flipVertical()
  {
    int linesize = m_width * m_pixelFormat.numChannels();

    int n = m_height / 2;

    for(int y = 0; y < n; y++) {
      uint8_t * l1 = line(y);
      uint8_t * l2 = line(m_height - y - 1);

      uint8_t * sentinel = l1 + linesize;

      while(l1 < sentinel) {
        uint8_t tmp = *l1;
        *l1 = *l2;
        *l2 = tmp;

        l1++;
        l2++;
      };
    }
  }

  bool Image::copyResample(const Image & source, int w, int h)
  {
    if(source.pixelFormat() == PixelFormat::rgbUByte()) {

      allocate(w, h, source.pixelFormat());

      int sw = source.width();
      int sh = source.height();

      float xscale = source.width()  / (float) w;
      float yscale = source.height() / (float) h;

      const uint8_t * src = source.bytes();

      for(int y = 0; y < h; y++) {

        float sy = y * yscale;
        int yi = (int) sy;
        int yi1 = yi + 1;

        if(yi1 >= sh) yi1 = yi;
        float wy1 = sy - yi;
        float wy0 = 1.0f - wy1;

        uint8_t * dest = bytes() + y * w * 3;

        for(int x = 0; x < w; x++) {

          float sx = x * xscale;
          int xi = (int) sx;
          int xi1 = xi + 1;
          if(xi1 >= sw) xi1 = xi;

          float wx1 = sx - xi;
          float wx0 = 1.0f - wx1;

          const uint8_t * v00 = & src[(yi * sw  + xi) * 3];
          const uint8_t * v10 = & src[(yi * sw  + xi1) * 3];
          const uint8_t * v01 = & src[(yi1 * sw + xi) * 3];
          const uint8_t * v11 = & src[(yi1 * sw + xi1) * 3];

          float fw00 = wy0 * wx0;
          float fw10 = wy0 * wx1;
          float fw01 = wy1 * wx0;
          float fw11 = wy1 * wx1;

          for(int c = 0; c < 3; c++) {
            float val =
                (*v00) * fw00 +  (*v10) * fw10 +  (*v01) * fw01 + (*v11) * fw11;

            *dest = uint8_t(Nimble::Math::Min((int) (val + 0.5f), 255));

            dest++;

            v00++;
            v10++;
            v01++;
            v11++;
          }
        }
      }
    }
    else if(source.pixelFormat() == PixelFormat::rgbaUByte()) {

      allocate(w, h, source.pixelFormat());

      int sw = source.width();
      int sh = source.height();

      float xscale = source.width()  / (float) w;
      float yscale = source.height() / (float) h;

      const uint8_t * src = source.bytes();
      for(int y = 0; y < h; y++) {

        float sy = y * yscale;
        int yi = (int) sy;
        int yi1 = yi + 1;

        if(yi1 >= sh) yi1 = yi;
        float wy1 = sy - yi;
        float wy0 = 1.0f - wy1;

        uint8_t * dest = bytes() + y * w * 4;

        for(int x = 0; x < w; x++) {

          float sx = x * xscale;
          int xi = (int) sx;
          int xi1 = xi + 1;
          if(xi1 >= sw) xi1 = xi;

          float wx1 = sx - xi;
          float wx0 = 1.0f - wx1;

          const uint8_t * v00 = & src[(yi * sw  + xi) * 4];
          const uint8_t * v10 = & src[(yi * sw  + xi1) * 4];
          const uint8_t * v01 = & src[(yi1 * sw + xi) * 4];
          const uint8_t * v11 = & src[(yi1 * sw + xi1) * 4];

          float fw00 = wy0 * wx0;
          float fw10 = wy0 * wx1;
          float fw01 = wy1 * wx0;
          float fw11 = wy1 * wx1;

          float a00 = v00[3];
          float a10 = v10[3];
          float a01 = v01[3];
          float a11 = v11[3];

          /*a00 = 200;
            a10 = 200;
            a01 = 200;
            a11 = 20;
            */
          float asum = a00 * fw00 + a10 * fw10 + a01 * fw01 + a11 * fw11;
          float ascale = asum > 0.00001 ? 1.0f / asum : 0.0f;

          for(int c = 0; c < 3; c++) {
            float val =
                ((*v00) * fw00 * a00 +  (*v10) * fw10 * a10 +
                 (*v01) * fw01 * a01 +  (*v11) * fw11 * a11) * ascale;

            *dest = uint8_t(Nimble::Math::Min((int) (val + 0.5f), 255));

            dest++;

            v00++;
            v10++;
            v01++;
            v11++;
          }

          *dest = uint8_t(Nimble::Math::Min((int) asum, 255));

          /* if(*dest > 128)
             printf("."); */
          // *dest = 255;
          dest++;
        }
      }

      //write("some-with-a.png");

      return true;
    }

    return false;
  }


  bool Image::quarterSize(const Image & source)
  {
    const int sw = source.width();
    int w = sw / 2;

    const int sh = source.height();
    int h = sh / 2;

    // Make even:
    /* if(w & 0x1)
       w--;
       if(h & 0x1)
       h--;
       */

    while(w & 0x3)
      w--;
    while(h & 0x3)
      h--;

    if(source.pixelFormat() == PixelFormat::alphaUByte() ||
       source.pixelFormat() == PixelFormat::luminanceUByte()) {

      allocate(w, h, source.pixelFormat());

      const uint8_t * src = source.bytes();

      for(int y = 0; y < h; y++) {

        const uint8_t * l0 = src + y * 2 * sw;
        const uint8_t * l1 = l0 + sw;

        uint8_t * dest = bytes() + y * w;

        for(int x = 0; x < w; x++) {

          unsigned tmp = l0[0];
          tmp += l0[1];
          tmp += l1[0];
          tmp += l1[1];

          *dest = uint8_t(tmp >> 2);

          l0 += 2;
          l1 += 2;

          dest++;
        }

      }

      return true;
    }
    else if(source.pixelFormat() == PixelFormat::rgbUByte()) {

      allocate(w, h, source.pixelFormat());

      const uint8_t * const src = source.bytes();

      for(int y = 0; y < h; y++) {

        const uint8_t * l0 = src + y * 2 * 3 * sw;
        const uint8_t * l1 = l0 + sw * 3;

        uint8_t * dest = bytes() + y * w * 3;

        for(int x = 0; x < w; x++) {

          for(int i = 0; i < 3; i++) {
            unsigned tmp = l0[0];
            tmp += l0[3];
            tmp += l1[0];
            tmp += l1[3];

            *dest = uint8_t(tmp >> 2);

            l0++;
            l1++;
            dest++;
          }

          l0 += 3;
          l1 += 3;
        }
      }

      return true;
    }
    else if(source.pixelFormat() == PixelFormat::rgbaUByte()) {

      allocate(w, h, source.pixelFormat());

      const uint8_t * const src = source.bytes();

      for(int y = 0; y < h; y++) {

        const uint8_t * l0 = src + y * 2 * 4 * sw;
        const uint8_t * l1 = l0 + sw * 4;

        uint8_t * dest = bytes() + y * w * 4;

        for(int x = 0; x < w; x++) {

          unsigned a00 = l0[3];
          unsigned a10 = l0[7];
          unsigned a01 = l1[3];
          unsigned a11 = l1[7];

          /* a00 = 255;
             a10 = 255;
             a01 = 255;
             a11 = 255;
             */
          unsigned asum = a00 + a10 + a01 + a11;

          if(!asum)
            asum = 1;

          for(int i = 0; i < 3; i++) {
            unsigned tmp = (unsigned) l0[0] * a00;
            tmp += (unsigned) l0[4] * a10;
            tmp += (unsigned) l1[0] * a01;
            tmp += (unsigned) l1[4] * a11;

            *dest = uint8_t(tmp / asum);

            l0++;
            l1++;
            dest++;
          }

          *dest = uint8_t(asum >> 2);
          // *dest = 255;
          dest++;

          l0 += 5;
          l1 += 5;
        }
      }

      return true;
    }

    return false;
  }

  bool Image::forgetLastPixels(int n)
  {
    if(n <= 0)
      return true;

    if(pixelFormat() == PixelFormat::rgbUByte()) {
      int linewidth = m_width * 3;
      int newlinewidth = (m_width - n) * 3;

      for(int y = 0; y < m_height; y++) {
        unsigned char * dest = bytes() + y * newlinewidth;
        unsigned char * src = bytes() + y * linewidth;

        memmove(dest, src, newlinewidth);
      }

      m_width -= n;

      return true;
    }

    return false;
  }

  void Image::forgetLastLines(int n)
  {

    if(m_height < n)
      m_height = 0;
    else
      m_height -= n;
  }

  void Image::forgetLastLine()
  {
    if(m_height) m_height--;
  }

  bool Image::hasAlpha() const
  {
    return (m_pixelFormat.layout() == PixelFormat::LAYOUT_ALPHA) ||
        (m_pixelFormat.layout() == PixelFormat::LAYOUT_LUMINANCE_ALPHA) ||
        (m_pixelFormat.layout() == PixelFormat::LAYOUT_RGBA);
  }

  void Image::makeValidTexture()
  {
    int xlose = width()  & 0x3;
    int ylose = height() & 0x3;

    forgetLastPixels(xlose);
    forgetLastLines(ylose);

  }


  Image& Image::operator = (const Image& img)
                           {
    allocate(img.m_width, img.m_height, img.m_pixelFormat);

    unsigned int bytes = m_width * m_height * m_pixelFormat.numChannels();
    memcpy(m_data, img.m_data, bytes);

    return *this;
  }

  void Image::allocate(int width, int height, const PixelFormat & pf)
  {
    unsigned int bytes = width * height * pf.numChannels();
    unsigned int mybytes = m_width * m_height * m_pixelFormat.numChannels();
    /*
    Radiant::debug("Image::allocate # PARAMS(%d, %d, %s) CURRENT(%d, %d, %s)", width, height, pf.toString().c_str(), m_width, m_height, m_pixelFormat.toString().c_str());
    Radiant::debug("\tbytes = %u, mybytes = %u", bytes, mybytes);
    */
    m_width = width;
    m_height = height;
    m_pixelFormat = pf;

    if(bytes != mybytes) {
      delete [] m_data;
      if(bytes)
        m_data = new unsigned char [bytes];
      else
        m_data = 0;
    }
  }
  /*
  // Guess the filetype from the extension
  static Image::ImageType typeFromFileExt(const std::string & filename)
  {
    Image::ImageType type = Image::IMAGE_TYPE_UNKNOWN;
    string ext = filename.substr(filename.rfind(".") + 1);

    if(strcasecmp(ext.c_str(), "tga") == 0) type = Image::IMAGE_TYPE_TGA;
    else if(strcasecmp(ext.c_str(), "png") == 0) type = Image::IMAGE_TYPE_PNG;
    else if(strcasecmp(ext.c_str(), "jpg") == 0 ||
        strcasecmp(ext.c_str(), "jpeg") == 0) type = Image::IMAGE_TYPE_JPG;

    return type;
  }
*/
  bool Image::read(const char* filename)
  {
    initDefaultImageCodecs();

    bool result = false;

    clear();

    FILE * file = fopen(filename, "rb");
    if(!file) {
      error("Image::read # failed to open file '%s'", filename);
      return false;
    }

    ImageCodec * codec = codecs()->getCodec(filename, file);
    if(codec) {
      result = codec->read(*this, file);
    } else {
      error("Image::read # no suitable codec found for '%s'", filename);
    }

    fclose(file);

    return result;
  }

  bool Image::write(const char* filename)
  {
    initDefaultImageCodecs();

    bool ret = false;

    FILE * file = fopen(filename, "wb");
    if(!file) {
      error("Image::write # failed to open file '%s'", filename);
      return false;
    }

    ImageCodec * codec = codecs()->getCodec(filename);
    if(codec) {
      ret = codec->write(*this, file);
    }
    else {
      error("Image::write # No codec for file '%s'", filename);
    }

    fclose(file);

    return ret;
  }

  void Image::fromData(const unsigned char * bytes, int width, int height,
                       PixelFormat format)
  {

    allocate(width, height, format);
    unsigned pixels = width * height;
    unsigned nbytes = pixels * format.numChannels();

    if(bytes)
      memcpy( & m_data[0], bytes, nbytes);
  }


  void Image::clear()
  {
    delete[] m_data;
    m_data = 0;

    m_width = 0;
    m_height = 0;
    m_pixelFormat = PixelFormat(PixelFormat::LAYOUT_UNKNOWN,
                                PixelFormat::TYPE_UNKNOWN);
  }
  /*
     void Image::scale(int reqWidth, int reqHeight, bool keepAspectRatio, Image& dest) const
     {
     dest.clear();

     if(empty()) {
     trace("Image::scaled # Scaling empty image");
     return;
     }
#if 1
dest = *this;
#else
  // Compute new dimensions
  int newWidth, newHeight;

  if(!keepAspectRatio) {
  newWidth = reqWidth;
  newHeight = reqHeight;
  } else {
  int rw = reqHeight * m_width / m_height;

  bool useHeight = (rw <= reqWidth);

  if(useHeight) {
  newWidth = rw;
  newHeight = reqHeight;
  } else {
  newHeight = reqWidth * m_height / m_width;
  newWidth = reqWidth;
  }
  }

  // No need to scale, just return a copy
  if(newWidth == m_width && newHeight == m_height) {
  dest = *this;
  return;
  }

  dest.allocate(newWidth, newHeight, m_pixelFormat);

  float xRatio = (float)m_width / (float)newWidth;
  float yRatio = (float)m_height / (float)newHeight;

  for(int y = 0; y < newHeight; y++) {
  for(int x = 0; x < newWidth; x++) {

  float sx = (float)x * xRatio;
  float sy = (float)y * yRatio;

  sample(sx, sy, xRatio, yRatio, dest, x, y);
  }
  }
#endif
}
*/
  /// @todo currently ignores alpha channel
  void Image::sample(float x1, float y1, float x2, float y2, Image & dest, int destX, int destY) const
  {
    int begX = (int)x1;
    int begY = (int)y1;
    int endX = (int)x2;
    int endY = (int)y2;

    int nc = m_pixelFormat.numChannels();

    for(int y = begY; y < endY; y++) {
      for(int x = begX; x < endX; x++) {

        float w = computeWeight(x, y, x1, y1, x2, y2);

        assert(w > 0.0f && w <= 1.0f);

        unsigned int dstOffset = destY * dest.width() + destX;
        unsigned int srcOffset = y * m_width + x;

        for(int c = 0; c < nc; c++)
          dest.m_data[nc * dstOffset + c] += (unsigned char)(w * m_data[nc * srcOffset + c]);
      }
    }
  }


  float Image::computeWeight(int x, int y, float x1, float y1, float x2, float y2) const
  {
    float sx = (x < x1 ? x1 : x);
    float sy = (y < y1 ? y1 : y);

    x++; y++;

    float ex = (x < x2 ? x : x2);
    float ey = (y < y2 ? y : y2);

    return (ex - sx) * (ey - sy);
  }

  bool Image::ping(const char * filename, ImageInfo & info)
  {
    bool result = false;

    FILE * file = fopen(filename, "rb");
    if(!file) {
      Radiant::error("Image::ping # failed to open file '%s' for reading.", filename);
      return result;
    }

    ImageCodec * codec = codecs()->getCodec(filename, file);
    if(codec) {
      result = codec->ping(info, file);
    } else {
		Radiant::error("No suitable image codec found for '%s'", filename);
	}

    fclose(file);

    return result;
  }

  void Image::bind(GLenum textureUnit, bool withmimaps)
  {
    Texture2D & tex = ref();
    tex.bind(textureUnit);

    if(tex.width() != width() ||
       tex.height() != height())
      tex.loadImage(*this, withmimaps);
  }

}
