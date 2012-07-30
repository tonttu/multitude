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

    LUMINOUS_API Texture(Texture && tex);
    LUMINOUS_API Texture & operator=(Texture && tex);

    /// @todo should we have our own enum for these?
    ///       PixelFormat doesn't have for example sized internal formats
    /// @param format 0 == automatic (default) or OpenGL internal format enum
    LUMINOUS_API void setInternalFormat(int format);
    LUMINOUS_API int internalFormat() const;

    LUMINOUS_API void setData(unsigned int width, const PixelFormat & dataFormat, const void * data);
    LUMINOUS_API void setData(unsigned int width, unsigned int height, const PixelFormat & dataFormat, const void * data);
    LUMINOUS_API void setData(unsigned int width, unsigned int height, unsigned int depth, const PixelFormat & dataFormat, const void * data);

    /// 0 == use width
    LUMINOUS_API void setLineSizePixels(std::size_t size);
    LUMINOUS_API unsigned int lineSizePixels() const;

    LUMINOUS_API bool isValid() const;

    LUMINOUS_API uint8_t dimensions() const;
    LUMINOUS_API unsigned int width() const;
    LUMINOUS_API unsigned int height() const;
    LUMINOUS_API unsigned int depth() const;
    LUMINOUS_API const PixelFormat & dataFormat() const;
    LUMINOUS_API const void * data() const;

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
