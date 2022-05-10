/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "MipMapGenerator.hpp"
#include "Image.hpp"
#include "ImageCodecDDS.hpp"
#include "Squish/squish.h"

#include <cassert>

namespace Luminous {

  MipMapGenerator::MipMapGenerator(const QString & src,
                                   const QString & target)
    : Task(defaultPriority()),
      m_src(src),
      m_target(target),
      m_out(0),
      m_flags(0)
  {
  }

  MipMapGenerator::MipMapGenerator(const QString & src,
                                   const QString & target,
                                   const PixelFormat & mipmapFormat)
    : Task(defaultPriority()),
      m_src(src),
      m_target(target),
      m_mipmapFormat(mipmapFormat),
      m_out(0),
      m_flags(0)
  {
  }

  MipMapGenerator::~MipMapGenerator()
  {
  }

  void MipMapGenerator::doTask()
  {
    if(m_mipmapFormat != PixelFormat() &&
       m_mipmapFormat.compression() == PixelFormat::COMPRESSION_NONE) {
      Radiant::error("MipMapGenerator::doTask # non-DXT -formats aren't supported");
      setFinished();
      if(m_listener) m_listener(false, ImageInfo());
      return;
    }

    Image img;
    if(!img.read(m_src, false)) {
      Radiant::error("MipMapGenerator::doTask # Failed to open %s", m_src.toUtf8().data());
      setFinished();
      if(m_listener) m_listener(false, ImageInfo());
      return;
    }

    if(m_mipmapFormat == PixelFormat()) {
      m_mipmapFormat = chooseMipmapFormat(img);
    }

    // Set the image format to 4 byte [RGBA] image
    if (!img.setPixelFormat(PixelFormat::rgbaUByte())) {
      Radiant::error("MipMapGenerator::doTask # Failed to convert the source image (%s) to RGBA pixel format",
                     m_src.toUtf8().data());
      setFinished();
      if (m_listener) m_listener(false, ImageInfo());
      return;
    }

    m_flags =
        m_mipmapFormat.compression() == PixelFormat::COMPRESSED_RGB_DXT1  ? squish::kDxt1 :
        m_mipmapFormat.compression() == PixelFormat::COMPRESSED_RGBA_DXT1 ? squish::kDxt1 :
        m_mipmapFormat.compression() == PixelFormat::COMPRESSED_RGBA_DXT3 ? squish::kDxt3 :
        m_mipmapFormat.compression() == PixelFormat::COMPRESSED_RGBA_DXT5 ? squish::kDxt5 : 0;

    assert(m_flags);

    // best quality, might be very slow
    //m_flags |= squish::kColourIterativeClusterFit;
    // high quality, slow (default at squish)
    //m_flags |= squish::kColourClusterFit;
    // fast, low quality
    m_flags |= squish::kColourRangeFit;

    // required size for DXT data, excluding all DDS headers
    int requiredSize = 0;
    // number of mipmaps, including the original one
    int mipmaps = 0;

    Nimble::Size size = img.size();
    for(;;) {
      requiredSize += ImageCodecDDS::linearSize(size, m_mipmapFormat.compression());
      ++mipmaps;
      if(size.width() <= 4 && size.height() <= 4) break;
      size /= 2;
    }

    m_outBuffer.resize(requiredSize);
    m_out = &m_outBuffer[0];

    resize(img, 0);

    ImageCodecDDS dds;
    dds.writeMipmaps(m_target, m_mipmapFormat.compression(),
                     img.size(), mipmaps, m_outBuffer);
    if(m_listener) {
      ImageInfo info;
      info.height = img.width();
      info.width = img.height();
      info.mipmaps = mipmaps;
      info.pf = m_mipmapFormat;
      m_listener(true, info);
    }
    setFinished();
  }

  void MipMapGenerator::setListener(std::function<void (bool, const ImageInfo &)> func)
  {
    m_listener = func;
  }

  void MipMapGenerator::resize(const Image & img, const int level)
  {
    const size_t raw_size = ImageCodecDDS::linearSize(img.size(), m_mipmapFormat.compression());

    // compress the image data as DXT to the end of the write buffer
    assert(m_out + raw_size <= &m_outBuffer[0] + m_outBuffer.size());
    squish::CompressImage(img.data(), img.width(), img.height(), m_out, m_flags);
    m_out += raw_size;

    if(img.width() > 4 || img.height() > 4) {
      // Resample next mipmap level
      Image mipmap;
      int w = img.width() >> 1, h = img.height() >> 1;
      //mipmap.copyResample(img, w ? w : 1, h ? h : 1);
      mipmap.minify(img, w ? w : 1, h ? h : 1);

      resize(mipmap, level + 1);
    }
  }

  PixelFormat MipMapGenerator::chooseMipmapFormat(const Image & img)
  {
    /// @todo some heuristic from the alpha channel histogram would be nice
    ///       - use DXT5 when there are alpha gradients
    if(img.pixelFormat().numChannels() == 4)
      return PixelFormat(PixelFormat::COMPRESSED_RGBA_DXT3);
    else
      return PixelFormat(PixelFormat::COMPRESSED_RGB_DXT1);
  }

  int MipMapGenerator::defaultPriority()
  {
    return (PRIORITY_NORMAL + PRIORITY_LOW) / 2;
  }
}
