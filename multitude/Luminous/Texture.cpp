/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/Texture.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/ContextArray.hpp"

#include <cassert>

namespace Luminous
{
  class Texture::D
  {
  public:
    void setData(uint8_t dimensions, unsigned int width, unsigned int height, unsigned int depth,
                 PixelFormat dataFormat, std::shared_ptr<const void> && data, bool allowAsyncUpload);

  public:
    uint8_t m_dimensions = 0;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_depth = 0;
    unsigned int m_samples = 0;
    // This is set to a valid format to avoid errors even if no data is specified for the texture
    PixelFormat m_dataFormat {PixelFormat::rgbUByte()};
    int m_internalFormat = 0;
    std::shared_ptr<const void> m_data;
    bool m_allowAsyncUpload = false;
    bool m_translucent = false;

    unsigned int m_lineSizeBytes;

    ContextArrayT<QRegion> m_dirtyRegions;
    Filter m_minFilter = FILTER_LINEAR;
    Filter m_magFilter = FILTER_LINEAR;
    Wrap m_wrap[3] { WRAP_CLAMP, WRAP_CLAMP, WRAP_CLAMP };
    Radiant::ColorPMA m_borderColor {0, 0, 0, 0};
    bool m_mipmapsEnabled = false;
    // Generation number for all glTexParameter-variables, min/magfilter, wrap, border
    int m_paramsGeneration = 0;
  };


  void Texture::D::setData(uint8_t dimensions, unsigned int width, unsigned int height, unsigned int depth,
                           PixelFormat dataFormat, std::shared_ptr<const void> && data, bool allowAsyncUpload)
  {
    m_dimensions = dimensions;
    m_width = width;
    m_height = height;
    m_depth = depth;
    m_dataFormat = dataFormat;
    m_data = std::move(data);
    m_allowAsyncUpload = allowAsyncUpload;
    m_translucent = dataFormat.hasAlpha();

    for (QRegion & dirty: m_dirtyRegions)
      dirty = QRegion();
  }

  /////////////////////////////////////////////////////////////////////////////

  Texture::Texture()
    : RenderResource(RenderResource::Texture)
    , m_d(new Texture::D())
  {
  }

  Texture::~Texture()
  {
  }

  Texture::Texture(const Texture & tex)
    : RenderResource(tex)
    , m_d(new Texture::D(*tex.m_d))
  {
  }

  Texture & Texture::operator=(const Texture & tex)
  {
    if(this != &tex)
    {
      RenderResource::operator=(tex);
      assert(tex.m_d);
      *m_d = *tex.m_d;
    }
    return *this;
  }

  Texture::Texture(Texture && tex)
    : RenderResource(std::move(tex))
    , m_d(std::move(tex.m_d))
  {
  }

  Texture & Texture::operator=(Texture && tex)
  {
    RenderResource::operator=(std::move(tex));
    std::swap(m_d, tex.m_d);
    return *this;
  }

  void Texture::setInternalFormat(int format)
  {
    if(m_d->m_internalFormat == format)
      return;
    m_d->m_internalFormat = format;
    invalidate();
  }

  int Texture::internalFormat() const
  {
    return m_d->m_internalFormat;
  }

  void Texture::setData(unsigned int width, const PixelFormat & dataFormat, const void * data)
  {
    m_d->setData(1, width, 1, 1, dataFormat, std::shared_ptr<const void>(data, [](const void*){}), false);
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, const void * data)
  {
    m_d->setData(2, width, height, 1, dataFormat, std::shared_ptr<const void>(data, [](const void*){}), false);
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, std::shared_ptr<const void> data)
  {
    m_d->setData(2, width, height, 1, dataFormat, std::move(data), true);
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & dataFormat, const void * data)
  {
    m_d->setData(3, width, height, depth, dataFormat, std::shared_ptr<const void>(data, [](const void*){}), false);
    invalidate();
  }

  void Texture::reset()
  {
    m_d->m_dimensions = 0;
    m_d->m_width = 0;
    m_d->m_height = 0;
    m_d->m_depth = 0;
    m_d->m_samples = 0;
    m_d->m_dataFormat = PixelFormat();
    m_d->m_translucent = false;
    m_d->m_data.reset();
    m_d->m_allowAsyncUpload = false;
  }

  std::size_t Texture::dataSize() const
  {
    auto comp = m_d->m_dataFormat.compression();
    if(comp == PixelFormat::COMPRESSION_NONE)
      return lineSizeBytes() * height() * depth();

    // align to 4 for compressed DXT textures
    int w = width(), h = height();
    w += (4 - (w & 3)) & 3;
    h += (4 - (h & 3)) & 3;

    switch(comp) {
    case PixelFormat::COMPRESSED_RGB_DXT1:
    case PixelFormat::COMPRESSED_RGBA_DXT1:
      return w * h / 2;

    //case PixelFormat::COMPRESSED_RGBA_DXT2:
    case PixelFormat::COMPRESSED_RGBA_DXT3:
    //case PixelFormat::COMPRESSED_RGBA_DXT4:
    case PixelFormat::COMPRESSED_RGBA_DXT5:
      return w * h;

    default:
      return 0;
    }
  }

  void Texture::setLineSizeBytes(unsigned int size)
  {
    if(m_d->m_lineSizeBytes == size)
      return;
    m_d->m_lineSizeBytes = size;
    invalidate();
  }

  unsigned int Texture::lineSizeBytes() const
  {
    return m_d->m_lineSizeBytes == 0 ? m_d->m_width * m_d->m_dataFormat.bytesPerPixel() : m_d->m_lineSizeBytes;
  }

  bool Texture::isValid() const
  {
    return m_d->m_dimensions >= 1 && m_d->m_dimensions <= 4;
  }

  uint8_t Texture::dimensions() const { return m_d->m_dimensions; }
  unsigned int Texture::width() const { return m_d->m_width; }
  unsigned int Texture::height() const { return m_d->m_height; }
  unsigned int Texture::depth() const { return m_d->m_depth; }
  const PixelFormat & Texture::dataFormat() const { return m_d->m_dataFormat; }
  const std::shared_ptr<const void> & Texture::data() const { return m_d->m_data; }

  QRegion Texture::dirtyRegion(unsigned int threadIndex) const
  {
    assert(threadIndex < m_d->m_dirtyRegions.size());
    return m_d->m_dirtyRegions[threadIndex];
  }

  QRegion Texture::takeDirtyRegion(unsigned int threadIndex) const
  {
    assert(threadIndex < (unsigned) m_d->m_dirtyRegions.size());
    QRegion r;
    std::swap(r, m_d->m_dirtyRegions[threadIndex]);
    return r;
  }

  void Texture::addDirtyRect(const QRect & rect)
  {
    auto intersected = rect.intersected(QRect(0, 0, width(), height()));
    for (QRegion & dirty: m_d->m_dirtyRegions)
      dirty += intersected;
  }

  unsigned int Texture::samples() const
  {
    return m_d->m_samples;
  }

  void Texture::setSamples(unsigned int samples)
  {
    m_d->m_samples = samples;
    invalidate();
  }

  bool Texture::translucent() const
  {
    return m_d->m_translucent;
  }

  void Texture::setTranslucency(bool translucency)
  {
    /// This is only used for batch rendering sorting optimization, no need to invalidate()
    m_d->m_translucent = translucency;
  }

  Texture::Filter Texture::getMinFilter() const
  {
    return m_d->m_minFilter;
  }

  void Texture::setMinFilter(Filter filter)
  {
    if (m_d->m_minFilter == filter)
      return;
    m_d->m_minFilter = filter;
    ++m_d->m_paramsGeneration;
  }

  Texture::Filter Texture::getMagFilter() const
  {
    return m_d->m_magFilter;
  }

  void Texture::setMagFilter(Filter filter)
  {
    if (m_d->m_magFilter == filter)
      return;
    m_d->m_magFilter = filter;
    ++m_d->m_paramsGeneration;
  }

  void Texture::setWrap(Wrap s, Wrap t, Wrap r)
  {
    if (m_d->m_wrap[0] == s && m_d->m_wrap[1] == t && m_d->m_wrap[2] == r)
      return;
    m_d->m_wrap[0] = s;
    m_d->m_wrap[1] = t;
    m_d->m_wrap[2] = r;
    ++m_d->m_paramsGeneration;
  }

  void Texture::getWrap(Wrap & s, Wrap & t, Wrap & r) const
  {
    s = m_d->m_wrap[0];
    t = m_d->m_wrap[1];
    r = m_d->m_wrap[2];
  }

  void Texture::setBorderColor(const Radiant::ColorPMA & color)
  {
    if (m_d->m_borderColor == color)
      return;
    m_d->m_borderColor = color;
    ++m_d->m_paramsGeneration;
  }

  const Radiant::ColorPMA & Texture::borderColor() const
  {
    return m_d->m_borderColor;
  }

  void Texture::setMipmapsEnabled(bool enabled)
  {
    if (m_d->m_mipmapsEnabled == enabled)
      return;
    m_d->m_mipmapsEnabled = enabled;
    invalidate();
  }

  bool Texture::mipmapsEnabled() const
  {
    return m_d->m_mipmapsEnabled;
  }

  int Texture::paramsGeneration() const
  {
    return m_d->m_paramsGeneration;
  }

  Texture::DataInfo Texture::dataInfo() const
  {
    DataInfo info;
    info.data = m_d->m_data;
    info.dataSize = dataSize();
    info.lineSizeBytes = lineSizeBytes();
    info.dataFormat = dataFormat();
    info.size.make(m_d->m_width, m_d->m_height, m_d->m_depth);
    return info;
  }

  bool Texture::allowAsyncUpload() const
  {
    return m_d->m_allowAsyncUpload;
  }
}
