#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/ContextArray.hpp"

#include <QCryptographicHash>

namespace
{
  template <typename T, typename Y>
  inline void set(bool & changed, T & target, const Y & src)
  {
    changed = changed || target != src;
    target = src;
  }
}

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

    bool needRehash;
    RenderResource::Hash hash;

    intptr_t externalKey;
    bool useExternalKey;

    ContextArrayT<QRegion> dirtyRegions;

  public:
    void rehash();
  };

  void Texture::D::rehash()
  {
    needRehash = false;
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData((const char*)&dimensions, sizeof(dimensions));
    hasher.addData((const char*)&width, sizeof(width));
    hasher.addData((const char*)&height, sizeof(height));
    hasher.addData((const char*)&depth, sizeof(depth));
    hasher.addData((const char*)&dataFormat, sizeof(dataFormat));
    hasher.addData((const char*)&internalFormat, sizeof(internalFormat));
    if(useExternalKey)
      hasher.addData((const char*)&externalKey, sizeof(externalKey));
    else
      hasher.addData((const char*)&data, sizeof(data));
    memcpy(&hash, hasher.result().data(), sizeof(hash));
  }

  Texture::Texture()
    : RenderResource(RenderResource::Texture)
    , m_d(new Texture::D())
  {
    m_d->needRehash = true;
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
    m_d->internalFormat = format;
    m_d->rehash();
  }

  int Texture::internalFormat() const
  {
    return m_d->internalFormat;
  }

  void Texture::setData(unsigned int width, const PixelFormat & dataFormat, const void * data)
  {
    set(m_d->needRehash, m_d->dimensions, 1);
    set(m_d->needRehash, m_d->width, width);
    set(m_d->needRehash, m_d->height, 1);
    set(m_d->needRehash, m_d->depth, 1);
    set(m_d->needRehash, m_d->dataFormat, dataFormat);
    set(m_d->needRehash, m_d->data, data);
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, const void * data)
  {
    set(m_d->needRehash, m_d->dimensions, 2);
    set(m_d->needRehash, m_d->width, width);
    set(m_d->needRehash, m_d->height, height);
    set(m_d->needRehash, m_d->depth, 1);
    set(m_d->needRehash, m_d->dataFormat, dataFormat);
    set(m_d->needRehash, m_d->data, data);
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & dataFormat, const void * data)
  {
    set(m_d->needRehash, m_d->dimensions, 3);
    set(m_d->needRehash, m_d->width, width);
    set(m_d->needRehash, m_d->height, height);
    set(m_d->needRehash, m_d->depth, depth);
    set(m_d->needRehash, m_d->dataFormat, dataFormat);
    set(m_d->needRehash, m_d->data, data);
    m_d->rehash();
    invalidate();
  }

  RenderResource::Hash Texture::hash() const
  {
    if(m_d->needRehash)
      m_d->rehash();

    return m_d->hash;
  }

  intptr_t Texture::externalKey() const
  {
    return m_d->externalKey;
  }

  void Texture::setExternalKey(intptr_t key)
  {
    if(m_d->useExternalKey) {
      set(m_d->needRehash, m_d->externalKey, key);
    } else {
      m_d->useExternalKey = true;
      m_d->externalKey = key;
      m_d->needRehash = true;
    }
  }

  void Texture::clearExternalKey()
  {
    if(!m_d->useExternalKey) {
      m_d->useExternalKey = false;
      m_d->needRehash = true;
    }
  }

  void Texture::setLineSizePixels(std::size_t size)
  {
    m_d->lineSizePixels = size;
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
