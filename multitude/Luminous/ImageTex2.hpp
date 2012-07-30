#ifndef LUMINOUS_IMAGETEX2_HPP
#define LUMINOUS_IMAGETEX2_HPP

#include "Luminous/Luminous.hpp"

#include <QString>

namespace Luminous
{
  class ImageTex2
  {
  public:
    LUMINOUS_API ImageTex2();
    LUMINOUS_API ~ImageTex2();
    LUMINOUS_API bool load(const QString & filename);
    LUMINOUS_API Texture & tex();

  private:
    class D;
    D * m_d;
  };
}

#endif // LUMINOUS_IMAGETEX2_HPP
