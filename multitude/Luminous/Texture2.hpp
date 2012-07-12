#if !defined (LUMINOUS_TEXTURE2_HPP)
#define LUMINOUS_TEXTURE2_HPP

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"

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

    LUMINOUS_API uint8_t dimensions() const;
    LUMINOUS_API unsigned int width() const;
    LUMINOUS_API unsigned int height() const;
    LUMINOUS_API unsigned int depth() const;
    LUMINOUS_API const PixelFormat & format() const;
    LUMINOUS_API const char * data() const;
  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_TEXTURE2_HPP
