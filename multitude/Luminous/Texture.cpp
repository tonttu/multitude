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
    /// Visual Studio 2010 doesn't null-initialize these (even though even the C++98 specs says it should *grmbl*)
    D() 
      : dimensions()
      , width()
      , height()
      , depth()
      , samples()
      // This is set to a valid format to avoid errors even if no data is specified for the texture
      , dataFormat(PixelFormat::rgbUByte())
      , internalFormat()
      , data()
      , translucent()
      , lineSizeBytes()
      , dirtyRegions()
      , m_minFilter(FILTER_LINEAR)
      , m_magFilter(FILTER_LINEAR)
      , m_borderColor(0, 0, 0, 0)
      , m_mipmapsEnabled(false)
      , m_paramsGeneration(0)
    {
      // Set default texture wrap modes
      m_wrap[0] = WRAP_CLAMP;
      m_wrap[1] = WRAP_CLAMP;
      m_wrap[2] = WRAP_CLAMP;
    }

    uint8_t dimensions;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int samples;
    PixelFormat dataFormat;
    int internalFormat;
    const void * data;
    bool translucent;

    unsigned int lineSizeBytes;

    ContextArrayT<QRegion> dirtyRegions;
    Filter m_minFilter, m_magFilter;
    Wrap m_wrap[3];
    Radiant::ColorPMA m_borderColor;
    bool m_mipmapsEnabled;
    // Generation number for all glTexParameter-variables, min/magfilter, wrap, border
    int m_paramsGeneration;
  };

  Texture::Texture()
    : RenderResource(RenderResource::Texture)
    , m_d(new Texture::D())
  {
  }

  Texture::~Texture()
  {
    delete m_d;
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
    , m_d(tex.m_d)
  {
    tex.m_d = nullptr;
  }

  Texture & Texture::operator=(Texture && tex)
  {
    RenderResource::operator=(std::move(tex));
    std::swap(m_d, tex.m_d);
    return *this;
  }

  void Texture::setInternalFormat(int format)
  {
    if(m_d->internalFormat == format)
      return;
    m_d->internalFormat = format;
    invalidate();
  }

  int Texture::internalFormat() const
  {
    return m_d->internalFormat;
  }

  void Texture::setData(unsigned int width, const PixelFormat & dataFormat, const void * data)
  {
    m_d->dimensions = 1;
    m_d->width = width;
    m_d->height = 1;
    m_d->depth = 1;
    m_d->dataFormat = dataFormat;
    m_d->data = data;
    m_d->translucent = dataFormat.hasAlpha();
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, const void * data)
  {
    m_d->dimensions = 2;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = 1;
    m_d->dataFormat = dataFormat;
    m_d->data = data;
    m_d->translucent = dataFormat.hasAlpha();
    for (QRegion & dirty: m_d->dirtyRegions)
      dirty = QRegion();
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & dataFormat, const void * data)
  {
    m_d->dimensions = 3;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = depth;
    m_d->dataFormat = dataFormat;
    m_d->data = data;
    m_d->translucent = dataFormat.hasAlpha();
    invalidate();
  }

  void Texture::reset()
  {
    m_d->dimensions = 0;
    m_d->width = 0;
    m_d->height = 0;
    m_d->depth = 0;
    m_d->samples = 0;
    m_d->dataFormat = PixelFormat();
    m_d->translucent = false;
    m_d->data = nullptr;
  }

  std::size_t Texture::dataSize() const
  {
    auto comp = m_d->dataFormat.compression();
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

  void Texture::setLineSizeBytes(std::size_t size)
  {
    if(m_d->lineSizeBytes == size)
      return;
    m_d->lineSizeBytes = size;
    invalidate();
  }

  unsigned int Texture::lineSizeBytes() const
  {
    return m_d->lineSizeBytes == 0 ? m_d->width * m_d->dataFormat.bytesPerPixel() : m_d->lineSizeBytes;
  }

  bool Texture::isValid() const
  {
    return m_d->dimensions >= 1 && m_d->dimensions <= 4;
  }

  uint8_t Texture::dimensions() const { return m_d->dimensions; }
  unsigned int Texture::width() const { return m_d->width; }
  unsigned int Texture::height() const { return m_d->height; }
  unsigned int Texture::depth() const { return m_d->depth; }
  const PixelFormat & Texture::dataFormat() const { return m_d->dataFormat; }
  const void * Texture::data() const { return m_d->data;}

  QRegion Texture::dirtyRegion(unsigned int threadIndex) const
  {
    assert(threadIndex < m_d->dirtyRegions.size());
    return m_d->dirtyRegions[threadIndex];
  }

  QRegion Texture::takeDirtyRegion(unsigned int threadIndex) const
  {
    assert(threadIndex < (unsigned) m_d->dirtyRegions.size());
    QRegion r;
    std::swap(r, m_d->dirtyRegions[threadIndex]);
    return r;
  }

  void Texture::addDirtyRect(const QRect & rect)
  {
    auto intersected = rect.intersected(QRect(0, 0, width(), height()));
    for (QRegion & dirty: m_d->dirtyRegions)
      dirty += intersected;
  }

  unsigned int Texture::samples() const
  {
    return m_d->samples;
  }

  void Texture::setSamples(unsigned int samples)
  {
    m_d->samples = samples;
    invalidate();
  }

  bool Texture::translucent() const
  {
    return m_d->translucent;
  }

  void Texture::setTranslucency(bool translucency)
  {
    /// This is only used for batch rendering sorting optimization, no need to invalidate()
    m_d->translucent = translucency;
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

}
