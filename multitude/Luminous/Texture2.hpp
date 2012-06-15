#if !defined (LUMINOUS_TEXTURE2_HPP)
#define LUMINOUS_TEXTURE_HPP

#include "Luminous/RenderResource.hpp"

namespace Luminous
{
  class Texture2 : public RenderResource
  {
  public:
    enum Filter {
      Filter_Nearest,
      Filter_Linear,
    };
    
  public:
    Texture2(RenderResource::Id id, RenderDriver & driver);
    ~Texture2();

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_TEXTURE_HPP
