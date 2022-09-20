/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "MipmapRenderer.hpp"
#include "RenderContext.hpp"

namespace Luminous
{
  bool MipmapRenderer::checkMipmaps(RenderContext & r, const Nimble::Rectf & rect, Mipmap & mipmap)
  {
    float blend;
    const unsigned int level = mipmap.level(r.transform(), rect.size(), r.maxTextureSize(), &blend);
    unsigned int level0 = (unsigned int)-1;
    auto tex0 = mipmap.texture(level, &level0);

    if (!tex0 || level != level0)
      return false;

    assert(tex0->data() != nullptr);

    if (blend > 0.0001f) {
      unsigned int level1;
      auto tex1 = mipmap.texture(level+1, &level1);

      if(tex1 && level1 == level+1) {
        // Trilinear filtering ok
        return true;
      }

      // Trilinear filtering failed
      return false;
    }

    // Normal bilinear filtering ok
    return true;
  }

  void MipmapRenderer::render(RenderContext & r, Style & style,
                              const Nimble::Rectf & rect, Mipmap & mipmap)
  {
    float blend;
    const unsigned int level = mipmap.level(r.transform(), rect.size(), r.maxTextureSize(), &blend);
    unsigned int level0 = (unsigned int)-1;
    auto tex0 = mipmap.texture(level, &level0);

    if (!tex0)
      return;

    assert(tex0->data() != nullptr);

    if (level0 == level && blend > 0.0001f) {
      unsigned int level1;
      auto tex1 = mipmap.texture(level+1, &level1);

      if(tex1 && level1 == level+1) {
        assert(tex1->data() != nullptr);
        // do trilinear filtering
        style.setFillProgram(r.trilinearTexShader());
        style.setTexture("tex[0]", *tex0);
        style.setTexture("tex[1]", *tex1);

        auto b = r.drawPrimitiveT<Luminous::BasicVertexUV, Luminous::TrilinearFilteringUniformBlock>(
              Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4,
              r.trilinearTexShader(), style.fillColor(), 1.f, style);
        b.vertex[0].location = rect.low();
        b.vertex[0].texCoord.make(0, 0);
        b.vertex[1].location = rect.highLow();
        b.vertex[1].texCoord.make(1, 0);
        b.vertex[2].location = rect.lowHigh();
        b.vertex[2].texCoord.make(0, 1);
        b.vertex[3].location = rect.high();
        b.vertex[3].texCoord.make(1, 1);

        b.uniform->blending = blend;
        return;
      }
    }

    // normal bilinear filtering
    style.setTexture(*tex0);
    r.drawRect(rect, style);
  }

} // namespace Luminous
