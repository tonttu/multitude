#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/ContextArray.hpp"

namespace Luminous
{
  class Texture::D
  {
  public:
    uint8_t dimensions;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    PixelFormat dataFormat;
    int internalFormat;
    const void * data;
    bool translucent;

    unsigned int lineSizePixels;

    ContextArrayT<QRegion> dirtyRegions;

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
    return m_d->dirtyRegions[threadIndex];
  }

  QRegion Texture::takeDirtyRegion(unsigned int threadIndex) const
  {
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
}
