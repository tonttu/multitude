#include "Luminous/Texture2.hpp"

namespace Luminous
{
  class Texture2::D
  {
  public:
  };

  Texture2::Texture2(RenderResource::Id id, RenderDriver & driver)
    : RenderResource(id, RT_Texture, driver)
    , m_d(new Texture2::D())
  {
  }

  Texture2::~Texture2()
  {
    delete m_d;
  }
}
