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
#include "Utils.hpp"
#include "Texture2.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/Trace.hpp>

#include <algorithm>
#include <cctype>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <QString>
#include <string.h>
#include <typeinfo>
#include <errno.h>

#ifdef RADIANT_LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#ifdef WIN32
#include <strings.h>	// strcasecmp()
#endif

#include <QFileInfo>
#include <QSettings>
#include <QDateTime>

using namespace std;
using namespace Radiant;

namespace
{
  bool fromMap(const QVariantMap & map, Luminous::ImageInfo & info)
  {
    info.width = map.value("width").toInt();
    info.height = map.value("height").toInt();
    info.mipmaps = map.value("mipmaps").toInt();
    if(map.contains("pf.compression")) {
      info.pf = Luminous::PixelFormat::Compression(map.value("pf.compression").toInt());
    } else {
      info.pf = Luminous::PixelFormat(
            Luminous::PixelFormat::ChannelLayout(map.value("pf.layout").toInt()),
            Luminous::PixelFormat::ChannelType(map.value("pf.type").toInt()));
    }
    return true;
  }

  QVariantMap toMap(const Luminous::ImageInfo & info)
  {
    QVariantMap map;
    map["width"] = info.width;
    map["height"] = info.height;
    map["mipmaps"] = info.mipmaps;
    if(info.pf.compression() != Luminous::PixelFormat::COMPRESSION_NONE) {
      map["pf.compression"] = info.pf.compression();
    } else {
      map["pf.layout"] = info.pf.layout();
      map["pf.type"] = info.pf.type();
    }
    return map;
  }

  bool doPing(const QString & filename, Luminous::ImageInfo & info)
  {
    bool result = false;

    FILE * file = fopen(filename.toUtf8().data(), "rb");
    if(!file) {
      Radiant::error("Image::ping # failed to open file '%s' for reading.", filename.toUtf8().data());
      return result;
    }

    Luminous::ImageCodec * codec = Luminous::Image::codecs()->getCodec(filename, file);
    if(codec) {
      result = codec->ping(info, file);
    } else {
      Radiant::error("No suitable image codec found for '%s'", filename.toUtf8().data());
    }

    fclose(file);

    return result;
  }
}

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
      : m_width(0),
      m_height(0),
      m_pixelFormat(PixelFormat::LAYOUT_UNKNOWN, PixelFormat::TYPE_UNKNOWN),
      m_data(0),
      m_generation(0)
      // m_dataReady(false),
      // m_ready(false)
  {}

  Image::Image(const Image& img)
      : m_width(0),
      m_height(0),
      m_pixelFormat(PixelFormat::LAYOUT_UNKNOWN, PixelFormat::TYPE_UNKNOWN),
      m_data(0),
      m_generation(0)
      // m_dataReady(false),
      // m_ready(false)
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
    int bpp = m_pixelFormat.bytesPerPixel();
    int linesize = m_width * bpp;

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
  }

  void Image::minify(const Image &src, int w, int h)
  {
    changed();

    allocate(w, h, src.pixelFormat());

    const float sx = float(src.width()) / float(w);
    const float sy = float(src.height()) / float(h);

    for(int y0 = 0; y0 < h; y0++) {

      for(int x0 = 0; x0 < w; x0++) {

        int count = 0;
        int alphaCount = 0;
        float alphaSum = 0.f;
        Nimble::Vector3 colorSum(0.f, 0.f, 0.f);

        for(int j = sy * y0; j < sy * y0 + sy; ++j) {
          for(int i = sx * x0; i < sx * x0 + sx; ++i) {
            Nimble::Vector4 p = src.pixel(i,j);
            // If this pixel contributes any color, scale by alpha and add it to the sum
            if (p.w > std::numeric_limits<float>::epsilon()) {
              alphaSum += p.w;
              colorSum += p.vector3() * p.w;
              ++alphaCount;
            }
            ++count;
          }
        }
        // Scale the color by the number of pixels that contribute
        colorSum /= alphaCount;
        // Scale the alpha by all the pixels
        alphaSum = std::min<float>(alphaSum / count, 1.f);

        setPixel(x0, y0, Nimble::Vector4(colorSum.x, colorSum.y, colorSum.z, alphaSum));
      }
    }
  }

  bool Image::copyResample(const Image & source, int w, int h)
  {
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


  /// @todo this function is retarded. It should be simplified.
  bool Image::quarterSize(const Image & source)
  {
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

      return true;
    }

    Radiant::error("Image::quarterSize # unsupported pixel format");
    return false;
  }

  bool Image::forgetLastPixels(int n)
  {
    if(n <= 0)
      return true;

    changed();

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

    changed();
  }

  void Image::forgetLastLine()
  {
    if(m_height) {
      m_height--;
      changed();
    }
  }

  bool Image::hasAlpha() const
  {
    return (m_pixelFormat.layout() == PixelFormat::LAYOUT_ALPHA) ||
        (m_pixelFormat.layout() == PixelFormat::LAYOUT_RED_GREEN) ||
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

    changed();

    return *this;
  }

  void Image::allocate(int width, int height, const PixelFormat & pf)
  {
    unsigned int bytes = width * height * pf.bytesPerPixel();
    unsigned int mybytes = m_width * m_height * m_pixelFormat.bytesPerPixel();

    if(width && height)
      assert(pf.numChannels() > 0);

    /*
    debugLuminous("Image::allocate # PARAMS(%d, %d, %s) CURRENT(%d, %d, %s)", width, height, pf.toString().toUtf8().data(), m_width, m_height, m_pixelFormat.toString().toUtf8().data());
    debugLuminous("\tbytes = %u, mybytes = %u", bytes, mybytes);
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

    changed();
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
  bool Image::read(const char* filename)
  {
    initDefaultImageCodecs();

    bool result = false;

    clear();

    FILE * file = fopen(filename, "rb");
    if(!file) {
      error("Image::read # failed to open file '%s'", filename);
      // m_ready = true;
      return false;
    }

    ImageCodec * codec = codecs()->getCodec(filename, file);
    if(codec) {
      result = codec->read(*this, file);
      // m_dataReady = result;
    } else {
      error("Image::read # no suitable codec found for '%s'", filename);
    }

    fclose(file);

    changed();

    // m_ready = true;

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
    } else {
      debugLuminous("Image::write # Could not deduce image codec based on filename '%s', using png", filename);

      codec = codecs()->getCodec(".png");
      ret = codec->write(*this, file);
    }

    fclose(file);

    return ret;
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
  }

  bool Image::setPixelFormat(const PixelFormat & format)
  {
    if(m_pixelFormat == format) return true;

    /// @todo finish this
    if(format.compression() != PixelFormat::COMPRESSION_NONE ||
       m_pixelFormat.compression() != PixelFormat::COMPRESSION_NONE ||
       format.type() != PixelFormat::TYPE_UBYTE ||
       m_pixelFormat.type() != PixelFormat::TYPE_UBYTE ||
       m_pixelFormat.layout() != PixelFormat::LAYOUT_RGB ||
       format.layout() != PixelFormat::LAYOUT_RGBA) {
      Radiant::error("Image::setPixelFormat # unsupported conversion");
      return false;
    }

    PixelFormat srcFormat = m_pixelFormat;
    uint8_t * src = m_data;
    m_data = 0;

    allocate(m_width, m_height, format);

    int src_bpp = srcFormat.bytesPerPixel();
    int dest_bpp = format.bytesPerPixel();
    for(int y = 0; y < m_height; ++y) {
      uint8_t * l = src + y * m_width * src_bpp;
      uint8_t * dest = bytes() + y * m_width * dest_bpp;

      for(int x = 0; x < m_width; ++x) {
        convert<PixelFormat::TYPE_UBYTE, PixelFormat::TYPE_UBYTE,
            PixelFormat::LAYOUT_RGB, PixelFormat::LAYOUT_RGBA>(
              format.numChannels(), l + x*src_bpp, dest + x*dest_bpp);
      }
    }

    if(src != m_data) delete [] src;
    return true;
  }

  void Image::clear()
  {
    delete[] m_data;
    m_data = 0;

    m_width = 0;
    m_height = 0;
    m_pixelFormat = PixelFormat(PixelFormat::LAYOUT_UNKNOWN,
                                PixelFormat::TYPE_UNKNOWN);
    changed();
  }

  bool Image::ping(const QString & filename, ImageInfo & info)
  {
    QFileInfo fi(filename);
    const QString & absFilename = fi.absoluteFilePath();
    if(absFilename.isEmpty()) {
      Radiant::error("Image::ping # File '%s' not found", filename.toUtf8().data());
      return false;
    }

    QSettings settings("MultiTouch", "ImageInfo");
    QVariantMap map = settings.value(absFilename).toMap();

    if(!map.isEmpty()) {
      QDateTime cacheModified = map.value("lastModified").toDateTime();
      if(cacheModified.isValid() && cacheModified >= fi.lastModified())
        return fromMap(map, info);
    }

    bool ok = doPing(absFilename, info);
    if(ok)
      settings.setValue(absFilename, toMap(info));
    return ok;
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
  }

  Nimble::Vector4 Image::safePixel(int x, int y) const
  {
    if(x < 0 || x >= width())
      return Nimble::Vector4(0.f, 0.f, 0.f, 0.f);

    if(y < 0 || y >= height())
      return Nimble::Vector4(0.f, 0.f, 0.f, 0.f);

    return pixel(x, y);
  }

  Nimble::Vector4 Image::pixel(unsigned x, unsigned y) const
  {
    if(empty())
      return Nimble::Vector4(0, 0, 0, 1);

    if(int(x) >= width())
      x = width() - 1;
    if(int(y) >= height())
      y = height() - 1;

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

    return Nimble::Vector4(0, 0, 0, 1);
  }

  void Image::setPixel(unsigned x, unsigned y, const Nimble::Vector4 &pixel)
  {
    assert(int(x) < width());
    assert(int(y) < height());

    uint8_t * px = line(y);
    px += pixelFormat().bytesPerPixel() * x;


    if(m_pixelFormat == PixelFormat::alphaUByte() || m_pixelFormat == PixelFormat::redUByte()) {
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
  }

  ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  ImageTex::ImageTex()
  {}

  bool ImageTex::bind(RenderContext * resources, GLenum textureUnit, bool withmipmaps, int internalFormat)
  {
    bool ret = true;

    Texture2D & tex = ref(resources);

    tex.bind(textureUnit);

    if(tex.generation() != generation()) {
      ret = tex.loadImage(textureUnit, *this, withmipmaps, internalFormat);
      tex.setGeneration(generation());
    }

    return ret;
  }

  bool ImageTex::isFullyLoadedToGPU(RenderContext * resources)
  {
    if(!width() || !height())
      return false;

    if(!resources)
      resources = RenderContext::getThreadContext();

    if(!resources->getResource(this))
      return false;

    Texture2D & tex = ref(resources);

    return (tex.m_uploadedLines == size_t(height()));
  }

  ImageTex * ImageTex::move()
  {
    ImageTex * t = new ImageTex;
    std::swap(t->m_width, m_width);
    std::swap(t->m_height, m_height);
    std::swap(t->m_pixelFormat, m_pixelFormat);
    std::swap(t->m_data, m_data);
    t->m_generation = ++m_generation;

    return t;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

#ifndef LUMINOUS_OPENGLES

  class CompressedImage::Private
  {
  public:
    Private(CompressedImage & img) : ptr(0), size(0), m_img(img) {}

    char * ptr;
    int size;

  private:
    CompressedImage & m_img;
  };

  CompressedImage::CompressedImage() : m_compression(PixelFormat::COMPRESSION_NONE), m_d(new Private(*this))
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

    FILE * file = fopen(filename.toUtf8().data(), "rb");
    if(!file) {
      error("CompressedImage::read # failed to open file '%s': %s", filename.toUtf8().data(), strerror(errno));
      return false;
    }

    ImageCodec * codec = Image::codecs()->getCodec(filename, file);
    if(codec) {
      result = codec->read(*this, file, level);
    } else {
      error("CompressedImage::read # no suitable codec found for '%s'", filename.toUtf8().data());
    }
    fclose(file);

    return result;
  }

  bool CompressedImage::loadImage(FILE * file, const ImageInfo & info, int size)
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

    if (fread(m_d->ptr, size, 1, file) != 1) {
      Radiant::error("CompressedImage::loadImage # Failed to read image");
      delete m_d->ptr;
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

      Vector2i blockid(pos.x / 4, pos.y / 4);

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

      Vector2i blockid(pos.x / 4, pos.y / 4);

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

  CompressedImageTex::~CompressedImageTex()
  {
  }

  void CompressedImageTex::bind(RenderContext * resources, GLenum textureUnit)
  {
    Texture2D & tex = ref(resources);
    tex.bind(textureUnit);

    // Luminous::Utils::glCheck("ImageTex::bind # 1");

    if(tex.width() != width() || tex.height() != height()) {
      tex.loadImage(*this);
    }
  }

  /// Creates a new CompressedImageTex from this, all the cpu data from
  /// Luminous::CompressedImage is moved to the new object.
  CompressedImageTex * CompressedImageTex::move()
  {
    CompressedImageTex * t = new CompressedImageTex;
    std::swap(t->m_size, m_size);
    std::swap(t->m_compression, m_compression);
    std::swap(t->m_d->ptr, m_d->ptr);
    std::swap(t->m_d->size, m_d->size);
    return t;
  }

#endif // LUMINOUS_OPENGLES

}
