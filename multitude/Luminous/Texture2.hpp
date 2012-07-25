#if !defined (LUMINOUS_TEXTURE2_HPP)
#define LUMINOUS_TEXTURE2_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

#include <QRegion>

namespace Luminous
{
  class Texture : public RenderResource
  {
  public:
    enum Filter {
      Filter_Nearest,
      Filter_Linear,
    };
    
  public:
    LUMINOUS_API Texture();
    LUMINOUS_API ~Texture();

    LUMINOUS_API void setData(unsigned int width, const PixelFormat & format, const char * data);
    LUMINOUS_API void setData(unsigned int width, unsigned int height, const PixelFormat & format, const char * data);
    LUMINOUS_API void setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & format, const char * data);

    /// @todo there should be a mode "shared" and "streaming"
    ///       the current implementation is "shared"
    ///       streaming means that data() shouldn't be used for hash, but some
    ///       other external key. also with streaming textures, generation
    ///       is used in the driver
    LUMINOUS_API Hash hash() const;

    LUMINOUS_API bool isValid() const;

    LUMINOUS_API uint8_t dimensions() const;
    LUMINOUS_API unsigned int width() const;
    LUMINOUS_API unsigned int height() const;
    LUMINOUS_API unsigned int depth() const;
    LUMINOUS_API const PixelFormat & format() const;
    LUMINOUS_API const char * data() const;

    LUMINOUS_API QRegion dirtyRegion(unsigned int threadIndex) const;
    LUMINOUS_API QRegion takeDirtyRegion(unsigned int threadIndex) const;
    LUMINOUS_API void addDirtyRect(const QRect & rect);

    LUMINOUS_API bool translucent() const;
    LUMINOUS_API void setTranslucency(bool translucency);

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_TEXTURE2_HPP
