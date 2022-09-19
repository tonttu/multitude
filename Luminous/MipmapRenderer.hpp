/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
