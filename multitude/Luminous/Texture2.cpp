#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"

namespace Luminous
{
  class Texture::D
  {
  public:
    uint8_t dimensions;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    PixelFormat format;
    const char * data;
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

  void Texture::setData(unsigned int width, const PixelFormat & format, const char * data)
  {
    m_d->dimensions = 1;
    m_d->width = width;
    m_d->height = m_d->depth = 1;
    m_d->format = format;
    m_d->data = data;
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, const PixelFormat & format, const char * data)
  {
    m_d->dimensions = 2;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = 1;
    m_d->format = format;
    m_d->data = data;
    invalidate();
  }

  void Texture::setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & format, const char * data)
  {
    m_d->dimensions = 3;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = depth;
    m_d->format = format;
    m_d->data = data;
    invalidate();
  }

  uint8_t Texture::dimensions() const { return m_d->dimensions; }
  unsigned int Texture::width() const { return m_d->width; }
  unsigned int Texture::height() const { return m_d->height; }
  unsigned int Texture::depth() const { return m_d->depth; }
  const PixelFormat & Texture::format() const { return m_d->format; }
  const char * Texture::data() const { return m_d->data;}
}
