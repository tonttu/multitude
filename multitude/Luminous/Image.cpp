
/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Image.hpp"
#include "ImageCodec.hpp"
#include "Luminous.hpp"
#include "CodecRegistry.hpp"
#include "Texture.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/Trace.hpp>

#include <algorithm>
#include <cctype>
#include <cassert>
#include <cmath>
#include <iostream>
#include <cstdint>
#include <stdlib.h>
#include <QString>
#include <string.h>
#include <typeinfo>
#include <errno.h>
#include <QFile>

#ifdef RADIANT_LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

namespace Luminous
{

  CodecRegistry * Image::codecs()
  {
    static CodecRegistry cr;

    return &cr;
  }

  template <PixelFormat::ChannelType type> struct PixelType;
  template <> struct PixelType<PixelFormat::TYPE_UBYTE> { typedef unsigned char Type; };

  template <PixelFormat::ChannelLayout layout> int get_index(int i);
  template <> int get_index<PixelFormat::LAYOUT_RGBA>(int i) { return i; }

  template <typename From, typename To> To convert_value(From from);
  template <> unsigned char convert_value(unsigned char x) { return x; }

  template <typename Type, PixelFormat::ChannelLayout layout> struct get_data {};
  template <typename Type> struct get_data<Type, PixelFormat::LAYOUT_RGB> {
    static Type get(const Type * t, int i) { return i < 3 ? t[i] : convert_value<unsigned char, Type>(255); }
  };
  template <typename Type> struct get_data<Type, PixelFormat::LAYOUT_BGR> {
    static Type get(const Type * t, int i) { return i < 3 ? t[2-i] : convert_value<unsigned char, Type>(255); }
  };
  template <typename Type> struct get_data<Type, PixelFormat::LAYOUT_RED> {
    static Type get(const Type * t, int i) { return i < 3 ? t[0] : convert_value<unsigned char, Type>(255); }
  };
  template <typename Type> struct get_data<Type, PixelFormat::LAYOUT_ALPHA> {
    static Type get(const Type * t, int i) { return i < 3 ? convert_value<unsigned char, Type>(255) : t[0]; }
  };

  template <PixelFormat::ChannelType TypeFrom, PixelFormat::ChannelType TypeTo,
            PixelFormat::ChannelLayout LayoutFrom, PixelFormat::ChannelLayout LayoutTo>
  void convert(int channels, uint8_t * l, uint8_t * dest) {
    typedef typename PixelType<TypeFrom>::Type FromT;
    typedef typename PixelType<TypeTo>::Type ToT;
    FromT * from = reinterpret_cast<FromT*>(l);
    ToT * to = reinterpret_cast<ToT*>(dest);

    for (int i = 0; i < channels; ++i)
      to[get_index<LayoutTo>(i)] = convert_value<FromT, ToT>(
            get_data<FromT, LayoutFrom>::get(from, i));
}

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Image::Image()
  {}

  Image::Image(const Image& img)
  {
    *this = img;
  }

  Image::Image(Image && img)
  {
    operator=(std::move(img));
  }

  Image::~Image()
  {
    clear();
  }

  void Image::flipVertical()
  {
    int linesize = lineSize();

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

    changed();

    if(m_texture)
      m_texture->addDirtyRect(QRect(0, 0, width(), height()));
  }

  void Image::minify(const Image &src, int w, int h)
  {
    changed();

    allocate(w, h, src.pixelFormat());

    const float sx = float(src.width()) / float(w);
    const float sy = float(src.height()) / float(h);

    const int bytesPerLine = lineSize();
    const int srcBytesPerLine = src.lineSize();

    if (hasPreMultipliedAlpha() || !hasAlpha()) {
      if (m_pixelFormat.type() == Luminous::PixelFormat::TYPE_UBYTE && m_pixelFormat.numChannels() == 3) {
        for (int y0 = 0; y0 < h; y0++) {
          uint8_t * targetPx = m_data + y0 * bytesPerLine;

          // Take 'floor' of the limits in order to avoid floating point accuracy problems
          const int maxY = sy * y0 + sy;

          for (int x0 = 0; x0 < w; x0++, targetPx += 3) {

            uint32_t colorSum[] = { 0, 0, 0 };

            const int maxX = sx * x0 + sx;

            int count = 0;
            for (int j = sy * y0; j < maxY; ++j) {
              const uint8_t * srcLine = src.m_data + j*srcBytesPerLine;
              for (int i = sx * x0; i < maxX; ++i) {
                colorSum[0] += srcLine[i * 3];
                colorSum[1] += srcLine[i * 3 + 1];
                colorSum[2] += srcLine[i * 3 + 2];
                ++count;
              }
            }

            const int round = count / 2;
            targetPx[0] = (colorSum[0] + round) / count;
            targetPx[1] = (colorSum[1] + round) / count;
            targetPx[2] = (colorSum[2] + round) / count;
          }
        }
      } else if (m_pixelFormat.type() == Luminous::PixelFormat::TYPE_UBYTE && m_pixelFormat.numChannels() == 4) {
        for (int y0 = 0; y0 < h; y0++) {
          uint8_t * targetPx = m_data + y0*bytesPerLine;

          // Take 'floor' of the limits in order to avoid floating point accuracy problems
          const int maxY = sy * y0 + sy;

          for (int x0 = 0; x0 < w; x0++, targetPx += 4) {

            uint32_t colorSum[] = { 0, 0, 0, 0 };

            const int maxX = sx * x0 + sx;

            int count = 0;
            for (int j = sy * y0; j < maxY; ++j) {
              const uint8_t * srcLine = src.m_data + j*srcBytesPerLine;
              for (int i = sx * x0; i < maxX; ++i) {
                colorSum[0] += srcLine[i * 4];
                colorSum[1] += srcLine[i * 4 + 1];
                colorSum[2] += srcLine[i * 4 + 2];
                colorSum[3] += srcLine[i * 4 + 3];
                ++count;
              }
            }

            const int round = count / 2;
            targetPx[0] = (colorSum[0] + round) / count;
            targetPx[1] = (colorSum[1] + round) / count;
            targetPx[2] = (colorSum[2] + round) / count;
            targetPx[3] = (colorSum[3] + round) / count;
          }
        }
      } else {

        for(int y0 = 0; y0 < h; y0++) {

          for(int x0 = 0; x0 < w; x0++) {

            int count = 0;
            Nimble::Vector4f colorSum(0.f, 0.f, 0.f, 0.f);

            // Take 'floor' of the limits in order to avoid floating point accuracy problems
            int maxY = sy * y0 + sy;
            int maxX = sx * x0 + sx;

            for(int j = sy * y0; j < maxY; ++j) {
              for(int i = sx * x0; i < maxX; ++i) {
                colorSum += src.pixel(i,j);
                ++count;
              }
            }

            // Round the color (setPixel just makes float -> int conversion wihtout rounding)
            colorSum += Nimble::Vector4f(1/512.0f, 1/512.0f, 1/512.0f, 1/512.0f);

            setPixel(x0, y0, colorSum / count);
          }
        }
      }
    } else {

      for(int y0 = 0; y0 < h; y0++) {

        for(int x0 = 0; x0 < w; x0++) {

          int count = 0;
          int alphaCount = 0;
          float alphaSum = 0.f;
          Nimble::Vector3 colorSum(0.f, 0.f, 0.f);

          // Take 'floor' of the limits in order to avoid floating point accuracy problems
          int maxY = sy * y0 + sy;
          int maxX = sx * x0 + sx;

          for(int j = sy * y0; j < maxY; ++j) {
            for(int i = sx * x0; i < maxX; ++i) {
              Nimble::Vector4f p = src.pixel(i,j);
              // If this pixel contributes any color, scale by alpha and add it to the sum
              if (p.w > std::numeric_limits<float>::epsilon()) {
                alphaSum += p.w;
                colorSum += p.vector3() * p.w;
                ++alphaCount;
              }
              ++count;
            }
          }
          if (alphaCount > 0) {
            colorSum /= alphaSum;
            alphaSum /= count;
            // Round the color (setPixel just makes float -> int conversion wihtout rounding)
            colorSum += Nimble::Vector3f(1/512.0f, 1/512.0f, 1/512.0f);
          }

          setPixel(x0, y0, Nimble::Vector4f(colorSum.x, colorSum.y, colorSum.z, alphaSum));
        }
      }
    }

    updateTexture();
  }

  bool Image::copyResample(const Image & source, int w, int h)
  {
    if (source.hasPreMultipliedAlpha()) {
      Radiant::error("Image::copyResample # Not implemented with pre-multiplied alpha");
    }
    changed();

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

            *dest = uint8_t(std::min((int) (val + 0.5f), 255));

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

          float asum = a00 * fw00 + a10 * fw10 + a01 * fw01 + a11 * fw11;
          float ascale = asum > 0.00001 ? 1.0f / asum : 0.0f;

          for(int c = 0; c < 3; c++) {
            float val =
                ((*v00) * fw00 * a00 +  (*v10) * fw10 * a10 +
                 (*v01) * fw01 * a01 +  (*v11) * fw11 * a11) * ascale;

            *dest = uint8_t(std::min((int) (val + 0.5f), 255));

            dest++;

            v00++;
            v10++;
            v01++;
            v11++;
          }

          *dest = uint8_t(std::min((int) asum, 255));

          /* if(*dest > 128)
             printf("."); */
          // *dest = 255;
          dest++;
        }
      }

      //write("some-with-a.png");

      updateTexture();

      return true;
    }

    return false;
  }


  /// @todo this function is retarded. It should be simplified.
  bool Image::quarterSize(const Image & source)
  {
    if (hasPreMultipliedAlpha()) {
      Radiant::error("Image::quarterSize # Not implemented with pre-multiplied alpha");
    }

    changed();

    const int sw = source.width();
    int w = sw / 2;

    const int sh = source.height();
    int h = sh / 2;

    if(source.pixelFormat() == PixelFormat::alphaUByte() ||
       source.pixelFormat() == PixelFormat::redUByte()) {

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

      updateTexture();

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

      updateTexture();

      return true;
    }
    else if(source.pixelFormat() == PixelFormat::rgbaUByte() || source.pixelFormat() == PixelFormat::bgraUByte()) {

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

      updateTexture();

      return true;
    }

    Radiant::error("Image::quarterSize # unsupported pixel format");

    return false;
  }

  bool Image::hasAlpha() const
  {
    return (m_pixelFormat.layout() == PixelFormat::LAYOUT_ALPHA) ||
        (m_pixelFormat.layout() == PixelFormat::LAYOUT_RED_GREEN) ||
        (m_pixelFormat.layout() == PixelFormat::LAYOUT_RGBA);
  }

  Image& Image::operator = (const Image& img)
  {
    // Required to avoid deadlock later in the function
    if(this == &img)
      return *this;

    // Copy image data
    allocate(img.m_width, img.m_height, img.m_pixelFormat);

    if (img.lineSize() == lineSize()) {
      unsigned int bytes = img.lineSize() * m_height;
      memcpy(m_data, img.m_data, bytes);
    } else {
      int bytesPerLine = lineSize();
      for (int y = 0; y < m_height; ++y)
        memcpy(line(y), img.line(y), bytesPerLine);
    }

    changed();

    // Copy associated texture (if any)
    if(img.m_texture) {
      Radiant::Guard g(img.m_textureMutex);

      if(img.m_texture) {
        // Copy the texture parameters...
        auto & tex = texture();
        tex = *img.m_texture;

        // ...but make sure the data points to the new image
        tex.setData(width(), height(), pixelFormat(), m_data);
        tex.setLineSizeBytes(m_lineSize);

      }
    } else {
      Radiant::Guard g(m_textureMutex);
      m_texture.reset();
    }

    return *this;
  }

  Image & Image::operator = (Image && img)
  {
    m_width = img.m_width;
    m_height = img.m_height;
    m_lineSize = img.m_lineSize;
    m_pixelFormat = img.m_pixelFormat;
    m_data = img.m_data;
    m_externalData = img.m_externalData;
    m_generation = img.m_generation;
    m_texture = std::move(img.m_texture);

    /// Invalidate the old image
    img.m_width = 0;
    img.m_height = 0;
    img.m_lineSize = 0;
    img.m_pixelFormat = Luminous::PixelFormat();
    img.m_data = nullptr;
    img.m_externalData = false;
    img.m_generation = 0;

    return *this;
  }

  void Image::allocate(int width, int height, const PixelFormat & pf)
  {
    unsigned int bytes = width * height * pf.bytesPerPixel();
    unsigned int mybytes = lineSize() * m_height;

    if(width && height)
      assert(pf.numChannels() > 0);

    /*
    debugLuminous("Image::allocate # PARAMS(%d, %d, %s) CURRENT(%d, %d, %s)", width, height, pf.toString().toUtf8().data(), m_width, m_height, m_pixelFormat.toString().toUtf8().data());
    debugLuminous("\tbytes = %u, mybytes = %u", bytes, mybytes);
    */
    m_width = width;
    m_height = height;
    m_pixelFormat = pf;
    m_lineSize = 0;

    if(bytes != mybytes) {
      if (!m_externalData)
        delete [] m_data;
      m_externalData = false;
      if(bytes)
        m_data = new unsigned char [bytes];
      else
        m_data = nullptr;
    }

    changed();

    updateTexture();
  }

  void Image::setData(unsigned char * bytes, int width, int height, PixelFormat format, int lineSize)
  {
    if (m_data && !m_externalData)
      delete [] m_data;

    m_data = bytes;
    m_externalData = true;
    m_width = width;
    m_height = height;
    m_pixelFormat = format;
    m_lineSize = lineSize;

    updateTexture();
  }
  /*
  // Guess the filetype from the extension
  static Image::ImageType typeFromFileExt(const QString & filename)
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
  bool Image::read(const QString & filename, bool usePreMultipliedAlpha)
  {
    initDefaultImageCodecs();

    clear();

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
      Radiant::error("Image::read # failed to open file '%s'", filename.toUtf8().data());
      return false;
    }

    auto codec = codecs()->getCodec(filename, &file);
    if (codec) {
      bool ok = false;
      try {
        ok = codec->read(*this, file);
      } catch (std::bad_alloc & err) {
        Radiant::error("Image::read # %s: %s", filename.toUtf8().data(), err.what());
      }

      if (!ok) {
        // Clean up in case the codec already modified something
        clear();
        return false;
      }
    } else {
      Radiant::error("Image::read # no suitable codec found for '%s'", filename.toUtf8().data());
      return false;
    }
    file.close();

    changed();

    if (usePreMultipliedAlpha) {
      toPreMultipliedAlpha();
    }

    updateTexture();

    return true;
  }

  bool Image::write(const QString & filename) const
  {
    initDefaultImageCodecs();

    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
      Radiant::error("Image::write # failed to open file '%s'", filename.toUtf8().data());
      return false;
    }

    std::shared_ptr<ImageCodec> codec = codecs()->getCodec(filename);
    if(!codec) {
      debugLuminous("Image::write # Could not deduce image codec based on filename '%s', using png", filename.toUtf8().data());

      codec = codecs()->getCodec(".png");
    }

    if (!codec) {
      Radiant::error("Image::write # failed to find any image codec");
      return false;
    }

    bool ok;
    if (hasPreMultipliedAlpha() && !codec->canWritePremultipliedAlpha()) {
      Luminous::Image tmp(*this);
      tmp.toPostMultipliedAlpha();
      ok = codec->write(tmp, file);
    } else {
      ok = codec->write(*this, file);
    }
    if (ok)
      return file.commit();
    return ok;
  }

  void Image::fromData(const unsigned char * bytes, int width, int height,
                       PixelFormat format)
  {

    allocate(width, height, format);
    unsigned pixels = width * height;
    unsigned nbytes = pixels * format.bytesPerPixel();


    if(bytes)
      memcpy( & m_data[0], bytes, nbytes);

    changed();

    updateTexture();
  }

  bool Image::setPixelFormat(const PixelFormat & format)
  {
    if(m_pixelFormat == format) return true;

    /// @todo finish this
    if(format.compression() != PixelFormat::COMPRESSION_NONE ||
       m_pixelFormat.compression() != PixelFormat::COMPRESSION_NONE ||
       format.type() != PixelFormat::TYPE_UBYTE ||
       m_pixelFormat.type() != PixelFormat::TYPE_UBYTE ||
       (m_pixelFormat.layout() != PixelFormat::LAYOUT_ALPHA &&
        m_pixelFormat.layout() != PixelFormat::LAYOUT_RED &&
        m_pixelFormat.layout() != PixelFormat::LAYOUT_RGB &&
        m_pixelFormat.layout() != PixelFormat::LAYOUT_BGR) ||
       format.layout() != PixelFormat::LAYOUT_RGBA) {
      Radiant::error("Image::setPixelFormat # unsupported conversion %s -> %s",
                     m_pixelFormat.toString().toUtf8().data(), format.toString().toUtf8().data());
      return false;
    }

    PixelFormat srcFormat = m_pixelFormat;
    int srcLineSize = lineSize();
    uint8_t * src = m_data;
    bool externalData = m_externalData;
    m_data = nullptr;

    allocate(m_width, m_height, format);

    int src_bpp = srcFormat.bytesPerPixel();
    int dest_bpp = format.bytesPerPixel();

#define CONVERT(srcLayout, targetLayout)                                  \
    for(int y = 0; y < m_height; ++y) {                                   \
      uint8_t * l = src + y * srcLineSize;                          \
      uint8_t * dest = bytes() + y * m_width * dest_bpp;                  \
      for(int x = 0; x < m_width; ++x) {                                  \
        convert<PixelFormat::TYPE_UBYTE, PixelFormat::TYPE_UBYTE,         \
            srcLayout, targetLayout>(                                     \
              format.numChannels(), l + x*src_bpp, dest + x*dest_bpp);    \
      }                                                                   \
    }

    if (srcFormat.layout() == PixelFormat::LAYOUT_RGB) {
      CONVERT(PixelFormat::LAYOUT_RGB, PixelFormat::LAYOUT_RGBA);
    } else {
      CONVERT(PixelFormat::LAYOUT_BGR, PixelFormat::LAYOUT_RGBA);
    }

    if(src != m_data && !externalData) delete [] src;

    // Check if the data needs to be converted. Need to override the
    // premultiplied alpha flag so that the conversion will not early out.
    if(format.isPremultipliedAlpha() && !srcFormat.isPremultipliedAlpha()) {
      m_pixelFormat.setPremultipliedAlpha(false);
      toPreMultipliedAlpha();
    } else if(!format.isPremultipliedAlpha() && srcFormat.isPremultipliedAlpha()) {
      m_pixelFormat.setPremultipliedAlpha(true);
      toPostMultipliedAlpha();
    }

    updateTexture();

    return true;
  }

  void Image::clear()
  {
    if (!m_externalData)
      delete[] m_data;
    m_data = nullptr;
    m_externalData = false;

    m_width = 0;
    m_height = 0;
    m_lineSize = 0;
    m_pixelFormat = PixelFormat(PixelFormat::LAYOUT_UNKNOWN,
                                PixelFormat::TYPE_UNKNOWN);

    changed();

    m_texture.reset();
  }

  bool Image::ping(const QString & filename, ImageInfo & info)
  {
    bool result = false;

    QFile file(filename);
    
    if(!file.open(QIODevice::ReadOnly)) {
      Radiant::error("Image::ping # failed to open file '%s' for reading.", filename.toUtf8().data());
      return result;
    }

    auto codec = Luminous::Image::codecs()->getCodec(filename, &file);
    if(codec) {
      result = codec->ping(info, file);
    } else {
      Radiant::error("No suitable image codec found for '%s'", filename.toUtf8().data());
    }

    file.close();

    return result;
  }

  unsigned char Image::pixelAlpha(Nimble::Vector2 relativeCoord) const
  {
      Nimble::Vector2i absolute(relativeCoord.x * width(), relativeCoord.y * height());

      size_t offset = absolute.y * width() + absolute.x;

      if(pixelFormat().numChannels() == 1)
        return m_data[offset];
      else if(pixelFormat().numChannels() == 4)
        return m_data[4 * offset + 3];

      return 255;
  }

  void Image::zero()
  {
    size_t byteCount = pixelFormat().bytesPerPixel() * width() * height();
    memset(data(), 0, byteCount);
    changed();

    if(m_texture)
      m_texture->addDirtyRect(QRect(0, 0, width(), height()));
  }

  Nimble::Vector4f Image::safePixel(int x, int y) const
  {
    if(x < 0 || x >= width())
      return Nimble::Vector4(0.f, 0.f, 0.f, 0.f);

    if(y < 0 || y >= height())
      return Nimble::Vector4(0.f, 0.f, 0.f, 0.f);

    return pixel(x, y);
  }

  Nimble::Vector4f Image::pixel(int x, int y) const
  {
    assert(m_data);
    assert(x >= 0 && x < width() && y >= 0 && y < height());

    const uint8_t * px = line(y);
    px += m_pixelFormat.bytesPerPixel() * x;

    const float s = 1.0f / 255.0f;

    // I guess alpha is special case
    if(m_pixelFormat == PixelFormat::alphaUByte() || m_pixelFormat == PixelFormat::redUByte())
      return Nimble::Vector4(1, 1, 1, px[0] * s);
    else if(m_pixelFormat.numChannels() == 3)
      return Nimble::Vector4(px[0] * s, px[1] * s, px[2] * s, 1);
    else if(m_pixelFormat.numChannels() == 4)
      return Nimble::Vector4(px[0] * s, px[1] * s, px[2] * s, px[3] * s);
    else {
      assert(0);
    }

    return Nimble::Vector4f(0, 0, 0, 1);
  }

  void Image::setPixel(unsigned x, unsigned y, const Nimble::Vector4f &pixel)
  {
    assert(int(x) < width());
    assert(int(y) < height());

    uint8_t * px = line(y);
    px += pixelFormat().bytesPerPixel() * x;


    if(m_pixelFormat == PixelFormat::redUByte()) {
        px[0] = pixel.x * 255;
    } else if(m_pixelFormat == PixelFormat::alphaUByte()) {
        px[0] = pixel.w * 255;
    } else if(pixelFormat().numChannels() == 3) {
        px[0] = pixel.x * 255;
        px[1] = pixel.y * 255;
        px[2] = pixel.z * 255;
    } else if(pixelFormat().numChannels() == 4) {
        px[0] = pixel.x * 255;
        px[1] = pixel.y * 255;
        px[2] = pixel.z * 255;
        px[3] = pixel.w * 255;
    } else {
      Radiant::error("Image::setPixel # unsupported pixel format");
      assert(0);
    }

    if(m_texture)
      m_texture->addDirtyRect(QRect(x, y, 1, 1));
  }

  Texture & Image::texture() const
  {
    return getTexture();
  }

  const Texture & Image::constTexture() const
  {
    return getTexture();
  }

  bool Image::hasTexture() const
  {
    // Doesn't need to lock the mutex, it makes no real difference
    return (m_texture != nullptr);
  }

  Texture &Image::getTexture() const
  {
    if(!m_texture) {
      Radiant::Guard g(m_textureMutex);
      if (!m_texture) {
        m_texture.reset(new Texture());
        m_texture->setData(width(), height(), pixelFormat(), m_data);
        m_texture->setLineSizeBytes(m_lineSize);
      }
    }

    return *m_texture.get();
  }

  void Image::updateTexture()
  {
    if (m_texture) {
      m_texture->setData(width(), height(), pixelFormat(), m_data);
      m_texture->setLineSizeBytes(m_lineSize);
    }
  }

  void Image::toPreMultipliedAlpha()
  {
    if (!hasAlpha() || hasPreMultipliedAlpha())
      return;

    if (m_pixelFormat.numChannels() == 4 && m_pixelFormat.bytesPerPixel() == 4) {
      // optimization to use integer arithmetic for most common pixel format
      for (int y = 0; y < m_height; ++y) {
        for (unsigned char * px = line(y), * end = px + m_width * 4; px != end; px += 4) {
          // +127 for rounding to correct direction
          px[0] = (px[0] * px[3] + 127) / 255;
          px[1] = (px[1] * px[3] + 127) / 255;
          px[2] = (px[2] * px[3] + 127) / 255;
        }
      }
    } else {
      // This should work for generic cases
      for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
          Nimble::Vector4f c = pixel(x, y);
          c.x *= c.w;
          c.y *= c.w;
          c.z *= c.w;
          setPixel(x, y, c);
        }
      }
    }
    m_pixelFormat.setPremultipliedAlpha(true);
  }

  void Image::toPostMultipliedAlpha()
  {
    if (!hasAlpha() || !hasPreMultipliedAlpha())
      return;

    if (m_pixelFormat.numChannels() == 4 && m_pixelFormat.bytesPerPixel() == 4) {
      // optimization to use integer arithmetic for most common pixel format
      for (int y = 0; y < m_height; ++y) {
        for (unsigned char * px = line(y), * end = px + m_width * 4; px != end; px += 4) {
          // if alpha is zero, then r,g,b is also zero, since this was pre-multiplied image
          // alpha>>1 is for rounding to the right direction
          int alpha = std::max<unsigned char>(px[3], 1);
          px[0] = ((alpha>>1) + px[0] * 255) / alpha;
          px[1] = ((alpha>>1) + px[1] * 255) / alpha;
          px[2] = ((alpha>>1) + px[2] * 255) / alpha;
        }
      }
    } else {
      // This should work for generic cases
      for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
          Nimble::Vector4f c = pixel(x, y);
          if (c.w > 0.0f) {
            c.x /= c.w;
            c.y /= c.w;
            c.z /= c.w;
          }
          setPixel(x, y, c);
        }
      }
    }
    m_pixelFormat.setPremultipliedAlpha(false);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

#ifndef LUMINOUS_OPENGLES

  class CompressedImage::Private
  {
  public:
    char * ptr = nullptr;
    int size = 0;
  };

  CompressedImage::CompressedImage() : m_compression(PixelFormat::COMPRESSION_NONE), m_d(new Private())
  {
  }

  CompressedImage::~CompressedImage()
  {
    clear();
    delete m_d;
  }

  void CompressedImage::clear()
  {
#if 0
    if(m_d->ptr) munmap(m_d->ptr, m_d->size + m_d->offset);
    if(m_d->fd) {
      counter(-1);
      close(m_d->fd);
    }
    m_d->ptr = 0;
    m_d->size = 0;
    m_d->offset = 0;
    m_d->fd = 0;
#endif
    if(m_d->ptr) delete[] m_d->ptr;
    m_d->ptr = 0;
    m_d->size = 0;
  }

  bool CompressedImage::read(const QString & filename, int level)
  {
    initDefaultImageCodecs();
    clear();

    bool result = false;

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
      Radiant::error("CompressedImage::read # failed to open file '%s': %s", filename.toUtf8().data(), strerror(errno));
      return false;
    }

    auto codec = Image::codecs()->getCodec(filename, &file);
    if(codec) {
      result = codec->read(*this, file, level);
    } else {
      Radiant::error("CompressedImage::read # no suitable codec found for '%s'", filename.toUtf8().data());
    }

    file.close();

    return result;
  }

  bool CompressedImage::loadImage(QFile & file, const ImageInfo & info, int size)
  {
    clear();
#if 0
    /// Ubuntu has ulimit -Hn == 1024, that is way too low for this kind of optimization
    static int pagesize = sysconf(_SC_PAGE_SIZE);
    int fd = dup(fileno(file));
    if(fd == -1) return false;
    lseek(fd, 0, SEEK_SET);

    int aligned_offset = offset / pagesize * pagesize;
    offset = offset - aligned_offset;
    void * ptr = mmap(NULL, size + offset, PROT_READ, MAP_SHARED, fd, aligned_offset);

    if(ptr == (void*)-1) {
      close(fd);
      return false;
    }
    m_d->ptr = reinterpret_cast<char*>(ptr);
    m_d->size = size;
    m_d->fd = fd;
    m_d->offset = offset;
#endif
    m_d->ptr = new char[size];

    auto bytesRead = file.read(m_d->ptr, size);
    if (bytesRead != size) {
      Radiant::error("CompressedImage::loadImage # Failed to read image");
      delete[] m_d->ptr;
      m_d->ptr = 0;
    }
    m_d->size = size;

    m_size.make(info.width, info.height);
    m_compression = info.pf.compression();
    return true;
  }

  void * CompressedImage::data() const
  {
#if 0
    return m_d->ptr + m_d->offset;
#endif
    return m_d->ptr;
  }

  int CompressedImage::datasize() const
  {
    return m_d->size;
  }

  float CompressedImage::readAlpha(Nimble::Vector2i pos) const
  {
    if(pos.x < 0 || pos.y < 0 || pos.x >= width() || pos.y >= height()) return 1.0f;

    if(m_compression == PixelFormat::COMPRESSED_RGBA_DXT1) {
      // This is not a standard format.
    } else if(m_compression == PixelFormat::COMPRESSED_RGBA_DXT3) {
      /* {
        Image img;
        img.allocate(width(), height(), PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE));
        unsigned char* out = img.bytes();
        const unsigned char* d = reinterpret_cast<const unsigned char*>(data());
        int blocksize = 16;
        int blocks_on_x_direction = (width() + 3) / 4;

        for(int x = 0; x < width(); ++x) {
          for(int y = 0; y < height(); ++y) {

            Vector2i blockid(x / 4, y / 4);

            const unsigned char* block = d + blocksize * (blockid.x + blockid.y * blocks_on_x_direction);

            int nibble_index = x % 4 + (y % 4) * 4;
            unsigned char byte = block[nibble_index/2];

            out[x+y*width()] = (nibble_index % 2) ? (byte >> 4) << 4 : (byte & 0xF) << 4;
          }
        }
        img.write("out.png");
      }*/
      const unsigned char* d = reinterpret_cast<const unsigned char*>(data());

      Nimble::Vector2i blockid(pos.x / 4, pos.y / 4);

      int blocksize = 16;
      int blocks_on_x_direction = (width() + 3) / 4;
      const unsigned char* block = d + blocksize * (blockid.x + blockid.y * blocks_on_x_direction);

      int nibble_index = pos.x % 4 + (pos.y % 4) * 4;
      unsigned char byte = block[nibble_index/2];

      byte = (nibble_index % 2) ? (byte >> 4) : (byte & 0xF);
      return byte / 16.0f;
    } else if(m_compression == PixelFormat::COMPRESSED_RGBA_DXT5) {
      // refer to http://en.wikipedia.org/wiki/S3_Texture_Compression
      const unsigned char* d = reinterpret_cast<const unsigned char*>(data());

      Nimble::Vector2i blockid(pos.x / 4, pos.y / 4);

      int blocksize = 16;
      int blocks_on_x_direction = (width() + 3) / 4;
      const unsigned char* block = d + blocksize * (blockid.x + blockid.y * blocks_on_x_direction);

      int nibble_index = pos.x % 4 + (pos.y % 4) * 4;

      unsigned char a0 = block[0];
      unsigned char a1 = block[1];

      uint64_t lookupTable = 0;
      for(int i = 0; i < 6; i++) {
        lookupTable <<=8;
        lookupTable ^= (int)block[2+i] & 0xFF;
      }
      uint64_t tripleBitMask = 0x7;
      tripleBitMask <<= 45 - nibble_index*3;

      lookupTable &= tripleBitMask;
      unsigned alphaIndex = (unsigned)(lookupTable >> (45 - nibble_index*3));

      if( alphaIndex == 0)
        return  a0/255.f;
      else if(alphaIndex == 1)
        return a1/255.f;
      else {
        float alpha;
        if(a0 > a1) {
          alpha = ((8-alphaIndex)*a0 + (alphaIndex-1)*a1)/(7*255.f);
        } else {
          if(alphaIndex == 7)
            alpha = 1.f;
          else if(alphaIndex == 6)
            alpha = 0.f;
          else
            alpha = ((6-alphaIndex)*a0 + (alphaIndex-1)*a1)/(5*255.f);
        }
        //      std::cout << std::dec << (int)a0 << " " << (int)a1 << " " << alpha << "\n";
        return alpha;
      }
    }

    return 1.0f;
  }

#endif // LUMINOUS_OPENGLES

}
