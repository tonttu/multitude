#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/ContextArray.hpp"

#include <QCryptographicHash>

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
    const char * data;
    bool translucent;
    RenderResource::Hash hash;

    ContextArrayT<QRegion> dirtyRegions;
  public:
    void rehash();
  };

  void Texture::D::rehash()
  {
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData((const char*)&dimensions, sizeof(dimensions));
    hasher.addData((const char*)&width, sizeof(width));
    hasher.addData((const char*)&height, sizeof(height));
    hasher.addData((const char*)&depth, sizeof(depth));
    hasher.addData((const char*)&dataFormat, sizeof(dataFormat));
    hasher.addData((const char*)&internalFormat, sizeof(internalFormat));
    hasher.addData((const char*)&data, sizeof(data));
    memcpy(&hash, hasher.result().data(), sizeof(hash));
  }

  Texture::Texture()
    : RenderResource(RenderResource::Texture)
    , m_d(new Texture::D())
  {
  }

  Texture::~Texture()
  {
    delete m_d;
  }

  void Texture::setInternalFormat(int format)
  {
    m_d->internalFormat = format;
    m_d->rehash();
  }

  int Texture::internalFormat() const
  {
    return m_d->internalFormat;
  }

  void Texture::setData(unsigned int width, const PixelFormat & dataFormat, const char * data)
  {
    m_d->dimensions = 1;
    m_d->width = width;
    m_d->height = m_d->depth = 1;
    m_d->dataFormat = dataFormat;
    m_d->data = data;
    m_d->rehash();
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, const char * data)
  {
    m_d->dimensions = 2;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = 1;
    m_d->dataFormat = dataFormat;
    m_d->data = data;
    m_d->rehash();
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & dataFormat, const char * data)
  {
    m_d->dimensions = 3;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = depth;
    m_d->dataFormat = dataFormat;
    m_d->data = data;
    m_d->rehash();
    invalidate();
  }

  RenderResource::Hash Texture::hash() const
  {
    return m_d->hash;
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
  const char * Texture::data() const { return m_d->data;}

  QRegion Texture::dirtyRegion(unsigned int threadIndex) const
  {
    return m_d->dirtyRegions[threadIndex];
  }

  QRegion Texture::takeDirtyRegion(unsigned int threadIndex) const
  {
    QRegion r = m_d->dirtyRegions[threadIndex];
    m_d->dirtyRegions[threadIndex] = QRegion();
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
    m_d->translucent = translucency;
  }
}
