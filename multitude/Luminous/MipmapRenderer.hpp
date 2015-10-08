#ifndef LUMINOUS_MIPMAPRENDERER_HPP
#define LUMINOUS_MIPMAPRENDERER_HPP

#include "Mipmap.hpp"

#include <Nimble/Rect.hpp>

namespace Luminous
{
  namespace MipmapRenderer
  {
    LUMINOUS_API bool checkMipmaps(RenderContext & r, const Nimble::Rectf & rect,
                                   Mipmap & mipmap);
    LUMINOUS_API void render(RenderContext & r, Style & style,
                             const Nimble::Rectf & rect, Mipmap & mipmap);
  }

} // namespace Luminous

#endif // LUMINOUS_MIPMAPRENDERER_HPP
