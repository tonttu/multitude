/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef POETIC_GPU_TEXTURE_GLYPH_HPP
#define POETIC_GPU_TEXTURE_GLYPH_HPP

#include "Export.hpp"
#include "Glyph.hpp"

#include <Luminous/Luminous.hpp>

namespace Poetic
{
  class CPUBitmapGlyph;

  /// A glyph stored in a texture on the GPU
  class POETIC_API GPUTextureGlyph : public Glyph
  {
    public:
      /// Constructs a new texture glyph
      GPUTextureGlyph(const CPUBitmapGlyph * glyph, int texId, int xOff, int yOff, GLsizei width, GLsizei height);
      virtual ~GPUTextureGlyph();

      virtual Nimble::Vector2 render(Nimble::Vector2 pen, const Nimble::Matrix3 & m, Nimble::Vector2f ** ptr);

    private:
      int m_width;
      int m_height;

      Nimble::Vector2 m_pos;
      Nimble::Vector2 m_uv[2];

      GLuint m_textureId;
  };

}

#endif
