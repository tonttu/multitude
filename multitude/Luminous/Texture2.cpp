#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"

namespace Luminous
{
  class Texture2::D
  {
  public:
    uint8_t dimensions;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    PixelFormat format;
    const char * data;
  };

  Texture2::Texture2(RenderResource::Id id, RenderDriver & driver)
    : RenderResource(id, ResourceType_Texture, driver)
    , m_d(new Texture2::D())
  {
  }

  Texture2::~Texture2()
  {
    delete m_d;
  }

  void Texture2::setData(unsigned int width, const PixelFormat & format, const char * data)
  {
    m_d->dimensions = 1;
    m_d->width = width;
    m_d->height = m_d->depth = 1;
    m_d->format = format;
    m_d->data = data;
    invalidate();
  }

  void Texture2::setData(unsigned int width, unsigned int height, const PixelFormat & format, const char * data)
  {
    m_d->dimensions = 2;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = 1;
    m_d->format = format;
    m_d->data = data;
    invalidate();
  }

  void Texture2::setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & format, const char * data)
  {
    m_d->dimensions = 3;
    m_d->width = width;
    m_d->height = height;
    m_d->depth = depth;
    m_d->format = format;
    m_d->data = data;
    invalidate();
  }

  uint8_t Texture2::dimensions() const { return m_d->dimensions; }
  unsigned int Texture2::width() const { return m_d->width; }
  unsigned int Texture2::height() const { return m_d->height; }
  unsigned int Texture2::depth() const { return m_d->depth; }
  const PixelFormat & Texture2::format() const { return m_d->format; }
  const char * Texture2::data() const { return m_d->data;}
}
