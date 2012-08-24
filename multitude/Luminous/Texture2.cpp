#include "Luminous/Texture2.hpp"
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
      // This is set to a valid format to avoid errors even if no data is specified for the texture
      , dataFormat(PixelFormat::rgbUByte())
      , internalFormat()
      , data()
      , translucent()
      , lineSizePixels()
      , dirtyRegions()
      , m_minFilter(Filter_Linear)
      , m_magFilter(Filter_Linear)
    {
      m_swizzleTargets[0] = Target_Red;
      m_swizzleTargets[1] = Target_Green;
      m_swizzleTargets[2] = Target_Blue;
      m_swizzleTargets[3] = Target_Alpha;
    }

    uint8_t dimensions;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    PixelFormat dataFormat;
    int internalFormat;
    const void * data;
    bool translucent;

    SwizzleTarget m_swizzleTargets[4];

    unsigned int lineSizePixels;

    ContextArrayT<QRegion> dirtyRegions;
    Filter m_minFilter, m_magFilter;

  public:
    void rehash();
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

  Texture::Texture(Texture & tex)
    : RenderResource(tex)
    , m_d(new Texture::D(*tex.m_d))
  {
  }

  Texture & Texture::operator=(Texture & tex)
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
    for(unsigned int i = 0; i < m_d->dirtyRegions.size(); ++i)
      m_d->dirtyRegions[i] = QRegion();
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
    invalidate();
  }

  void Texture::reset()
  {
    m_d->dimensions = 0;
    m_d->width = 0;
    m_d->height = 0;
    m_d->depth = 0;
    m_d->dataFormat = PixelFormat();
    m_d->data = nullptr;
  }

  std::size_t Texture::dataSize() const
  {
    auto comp = m_d->dataFormat.compression();
    if(comp == PixelFormat::COMPRESSION_NONE)
      return m_d->dataFormat.bytesPerPixel() * lineSizePixels() * height();

    // align to 4
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

  void Texture::setLineSizePixels(std::size_t size)
  {
    if(m_d->lineSizePixels == size)
      return;
    m_d->lineSizePixels = size;
    invalidate();
  }

  unsigned int Texture::lineSizePixels() const
  {
    return m_d->lineSizePixels == 0 ? m_d->width : m_d->lineSizePixels;
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
    assert(threadIndex < m_d->dirtyRegions.size());
    QRegion r;
    std::swap(r, m_d->dirtyRegions[threadIndex]);
    return r;
  }

  void Texture::addDirtyRect(const QRect & rect)
  {
    auto intersected = rect.intersected(QRect(0, 0, width(), height()));
    for(unsigned int i = 0; i < m_d->dirtyRegions.size(); ++i)
      m_d->dirtyRegions[i] += intersected;
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

  void Texture::setSwizzle(unsigned channel, SwizzleTarget target)
  {
    m_d->m_swizzleTargets[channel] = target;
    invalidate();
  }

  Texture::SwizzleTarget Texture::getSwizzle(unsigned channel) const
  {
    assert(channel <= 3);
    return m_d->m_swizzleTargets[channel];
  }


  Texture::Filter Texture::getMinFilter() const
  {
    return m_d->m_minFilter;
  }

  void Texture::setMinFilter(Filter filter)
  {
    m_d->m_minFilter = filter;
    invalidate();
  }

  Texture::Filter Texture::getMagFilter() const
  {
    return m_d->m_magFilter;
  }

  void Texture::setMagFilter(Filter filter)
  {
    m_d->m_magFilter = filter;
    invalidate();
  }
}
